#include "App.h"
#include "Render.h"
#include "Textures.h"
#include "Map.h"
#include "Physics.h"
#include "Entity.h"
#include "EntityManager.h"

#include "Defs.h"
#include "Log.h"
#include "Optick/include/optick.h"

#include <math.h>
#include "SDL_image/include/SDL_image.h"

Map::Map() : Module(), mapLoaded(false)
{
    name.Create("map");
}

// Destructor
Map::~Map()
{}

// Called before render is available
bool Map::Awake(pugi::xml_node& config)
{
    LOG("Loading Map Parser");
    bool ret = true;

    mapFileName = config.child("mapfile").attribute("path").as_string();
    mapFolder = config.child("mapfolder").attribute("path").as_string();

    return ret;
}

bool Map::CreateWalkabilityMap(int& width, int& height, uchar** buffer) const
{
    bool ret = false;
    ListItem<MapLayer*>* item;
    item = mapData.maplayers.start;

    for (item = mapData.maplayers.start; item != NULL; item = item->next)
    {
        MapLayer* layer = item->data;

        if (layer->properties.GetProperty("Navigation") != NULL && !layer->properties.GetProperty("Navigation")->value)
            continue;

        uchar* map = new uchar[layer->width * layer->height];
        memset(map, 1, layer->width * layer->height);

        for (int y = 0; y < mapData.height; ++y)
        {
            for (int x = 0; x < mapData.width; ++x)
            {
                int i = (y * layer->width) + x;

                int tileId = layer->Get(x, y);
                TileSet* tileset = (tileId > 0) ? GetTilesetFromTileId(tileId) : NULL;

                if (tileset != NULL)
                {
                    //According to the mapType use the ID of the tile to set the walkability value
                    if (mapData.type == MapTypes::MAPTYPE_ISOMETRIC && tileId == 25) map[i] = 1;
                    else if (mapData.type == MapTypes::MAPTYPE_ORTHOGONAL && tileId == 296) map[i] = 1;
                    else map[i] = 0;
                }
                else {
                    //LOG("CreateWalkabilityMap: Invalid tileset found");
                    map[i] = 0;
                }
            }
        }

        *buffer = map;
        width = mapData.width;
        height = mapData.height;
        ret = true;

        break;
    }

    return ret;
}

void Map::Draw()
{
    OPTICK_EVENT();
    if(mapLoaded == false)
        return;

    ListItem<MapLayer*>* mapLayerItem;
    mapLayerItem = mapData.maplayers.start;

    while (mapLayerItem != NULL) {

        //L06: DONE 7: use GetProperty method to ask each layer if your �Draw� property is true.
        if (mapLayerItem->data->properties.GetProperty("Draw") != NULL && mapLayerItem->data->properties.GetProperty("Draw")->value) {

            for (int x = 0; x < mapLayerItem->data->width; x++)
            {
                for (int y = 0; y < mapLayerItem->data->height; y++)
                {
                    // L05: DONE 9: Complete the draw function
                    int gid = mapLayerItem->data->Get(x, y);

                    //L06: DONE 3: Obtain the tile set using GetTilesetFromTileId
                    TileSet* tileset = GetTilesetFromTileId(gid);

                    SDL_Rect r = tileset->GetTileRect(gid);
                    iPoint pos = MapToWorld(x, y);

                    app->render->DrawTexture(tileset->texture,
                        pos.x,
                        pos.y,
                        &r);
                }
            }
        }
        mapLayerItem = mapLayerItem->next;

    }
}

iPoint Map::MapToWorld(int x, int y) const
{
    iPoint ret;

    if (mapData.type == MAPTYPE_ORTHOGONAL)
    {
        ret.x = x * mapData.tileWidth;
        ret.y = y * mapData.tileHeight;
    }
    else if (mapData.type == MAPTYPE_ISOMETRIC)
    {
        ret.x = (x - y) * (mapData.tileWidth / 2);
        ret.y = (x + y) * (mapData.tileHeight / 2);
    }

    return ret;
}

iPoint Map::WorldToMap(int x, int y)
{
    iPoint ret(0, 0);

    if (mapData.type == MAPTYPE_ORTHOGONAL)
    {
        ret.x = x / mapData.tileWidth;
        ret.y = y / mapData.tileHeight;
    }
    else if (mapData.type == MAPTYPE_ISOMETRIC)
    {
        float halfWidth = mapData.tileWidth * 0.5f;
        float halfHeight = mapData.tileHeight * 0.5f;
        ret.x = int((x / halfWidth + y / halfHeight) / 2);
        ret.y = int((y / halfHeight - x / halfWidth) / 2);
    }
    else
    {
        LOG("Unknown map type");
        ret.x = x; ret.y = y;
    }

    return ret;
}

// Get relative Tile rectangle
SDL_Rect TileSet::GetTileRect(int gid) const
{
    SDL_Rect rect = { 0 };
    int relativeIndex = gid - firstgid;

    // L05: DONE 7: Get relative Tile rectangle
    rect.w = tileWidth;
    rect.h = tileHeight;
    rect.x = margin + (tileWidth + spacing) * (relativeIndex % columns);
    rect.y = margin + (tileWidth + spacing) * (relativeIndex / columns);

    return rect;
}


// L06: DONE 2: Pick the right Tileset based on a tile id
TileSet* Map::GetTilesetFromTileId(int gid) const
{
    ListItem<TileSet*>* item = mapData.tilesets.start;
    TileSet* set = NULL;

    while (item)
    {
        set = item->data;
        if (gid < (item->data->firstgid + item->data->tilecount))
        {
            break;
        }
        item = item->next;
    }

    return set;
}

// Called before quitting
bool Map::CleanUp()
{
    LOG("Unloading map");

	ListItem<TileSet*>* item;
	item = mapData.tilesets.start;

	while (item != NULL)
	{
		RELEASE(item->data);
		item = item->next;
	}
	mapData.tilesets.Clear();

    // Remove all layers
    ListItem<MapLayer*>* layerItem;
    layerItem = mapData.maplayers.start;

    while (layerItem != NULL)
    {
        RELEASE(layerItem->data);
        layerItem = layerItem->next;
    }

    Colliders.Clear();

    return true;
}

// Load new map
bool Map::Load()
{
    bool ret = true;

    pugi::xml_document mapFileXML;
    pugi::xml_parse_result result = mapFileXML.load_file(mapFileName.GetString());

    if(result == NULL)
    {
        LOG("Could not load map xml file %s. pugi error: %s", mapFileName, result.description());
        ret = false;
    }

    if(ret == true)
    {
        ret = LoadMap(mapFileXML);
    }

    if (ret == true)
    {
        ret = LoadTileSet(mapFileXML);
    }

    // L05: DONE 4: Iterate all layers and load each of them
    if (ret == true)
    {
        ret = LoadAllLayers(mapFileXML.child("map"));
    }

    if (ret == true)
    {
        ret = LoadObject(mapFileXML.child("map"));
    }

    if(ret == true)
    {       
        LOG("Successfully parsed map XML file :%s", mapFileName.GetString());
        LOG("width : %d height : %d",mapData.width,mapData.height);
        LOG("tile_width : %d tile_height : %d",mapData.tileWidth, mapData.tileHeight);
        
        LOG("Tilesets----");

        ListItem<TileSet*>* tileset;
        tileset = mapData.tilesets.start;

        while (tileset != NULL) {
            LOG("name : %s firstgid : %d",tileset->data->name.GetString(), tileset->data->firstgid);
            LOG("tile width : %d tile height : %d", tileset->data->tileWidth, tileset->data->tileHeight);
            LOG("spacing : %d margin : %d", tileset->data->spacing, tileset->data->margin);
            tileset = tileset->next;
        }

        ListItem<MapLayer*>* mapLayer;
        mapLayer = mapData.maplayers.start;

        while (mapLayer != NULL) {
            LOG("id : %d name : %s", mapLayer->data->id, mapLayer->data->name.GetString());
            LOG("Layer width : %d Layer height : %d", mapLayer->data->width, mapLayer->data->height);
            mapLayer = mapLayer->next;
        }
    }

    if(mapFileXML) mapFileXML.reset();

    mapLoaded = ret;

    return ret;
}

bool Map::LoadMap(pugi::xml_node mapFile)
{
    bool ret = true;
    pugi::xml_node map = mapFile.child("map");

    if (map == NULL)
    {
        LOG("Error parsing map xml file: Cannot find 'map' tag.");
        ret = false;
    }
    else
    {
        //Load map general properties
        mapData.height = map.attribute("height").as_int();
        mapData.width = map.attribute("width").as_int();
        mapData.tileHeight = map.attribute("tileheight").as_int();
        mapData.tileWidth = map.attribute("tilewidth").as_int();
        mapData.type = MAPTYPE_UNKNOWN;

        mapData.type = MAPTYPE_UNKNOWN;
        if (strcmp(map.attribute("orientation").as_string(), "isometric") == 0)
        {
            mapData.type = MAPTYPE_ISOMETRIC;
        }
        if (strcmp(map.attribute("orientation").as_string(), "orthogonal") == 0)
        {
            mapData.type = MAPTYPE_ORTHOGONAL;
        }
    }

    return ret;
}

// L04: DONE 4: Implement the LoadTileSet function to load the tileset properties
bool Map::LoadTileSet(pugi::xml_node mapFile){

    bool ret = true; 

    pugi::xml_node tileset;
    for (tileset = mapFile.child("map").child("tileset"); tileset && ret; tileset = tileset.next_sibling("tileset"))
    {
        TileSet* set = new TileSet();

        // L04: DONE 4: Load Tileset attributes
        set->name = tileset.attribute("name").as_string();
        set->firstgid = tileset.attribute("firstgid").as_int();
        set->margin = tileset.attribute("margin").as_int();
        set->spacing = tileset.attribute("spacing").as_int();
        set->tileWidth = tileset.attribute("tilewidth").as_int();
        set->tileHeight = tileset.attribute("tileheight").as_int();
        set->columns = tileset.attribute("columns").as_int();
        set->tilecount = tileset.attribute("tilecount").as_int();

        // L04: DONE 4: Load Tileset image
        SString tmp("%s%s", mapFolder.GetString(), tileset.child("image").attribute("source").as_string());
        set->texture = app->tex->Load(tmp.GetString());

        mapData.tilesets.Add(set);
    }

    return ret;
}

// L05: DONE 3: Implement a function that loads a single layer layer
bool Map::LoadLayer(pugi::xml_node& node, MapLayer* layer)
{
    bool ret = true;

    //Load the attributes
    layer->id = node.attribute("id").as_int();
    layer->name = node.attribute("name").as_string();
    layer->width = node.attribute("width").as_int();
    layer->height = node.attribute("height").as_int();

    //L06: DONE 6 Call Load Propoerties
    LoadProperties(node, layer->properties);

    //Reserve the memory for the data 
    layer->data = new uint[layer->width * layer->height];
    memset(layer->data, 0, layer->width * layer->height);

    //Iterate over all the tiles and assign the values
    pugi::xml_node tile;
    int i = 0;
    for (tile = node.child("data").child("tile"); tile && ret; tile = tile.next_sibling("tile"))
    {
        layer->data[i] = tile.attribute("gid").as_int();
        i++;
    }

    return ret;
}

// L05: DONE 4: Iterate all layers and load each of them
bool Map::LoadAllLayers(pugi::xml_node mapNode) {
    bool ret = true;

    for (pugi::xml_node layerNode = mapNode.child("layer"); layerNode && ret; layerNode = layerNode.next_sibling("layer"))
    {
        //Load the layer
        MapLayer* mapLayer = new MapLayer();
        ret = LoadLayer(layerNode, mapLayer);

        //add the layer to the map
        mapData.maplayers.Add(mapLayer);
    }

    return ret;
}

// L06: DONE 6: Load a group of properties from a node and fill a list with it
bool Map::LoadProperties(pugi::xml_node& node, Properties& properties)
{
    bool ret = false;

    for (pugi::xml_node propertieNode = node.child("properties").child("property"); propertieNode; propertieNode = propertieNode.next_sibling("property"))
    {
        Properties::Property* p = new Properties::Property();
        p->name = propertieNode.attribute("name").as_string();
        p->value = propertieNode.attribute("value").as_bool(); // (!!) I'm assuming that all values are bool !!

        properties.list.Add(p);
    }

    return ret;
}

bool Map::LoadObject(pugi::xml_node node)
{
    bool ret = true;
    LOG("Loading Objects");

    ColLayer object;
    for (pugi::xml_node colNode = node.child("objectgroup"); colNode && ret; colNode = colNode.next_sibling("objectgroup"))
    {
        //printf("NAME %s", colNode.attribute("name").as_string());
        if (colNode.attribute("id").as_int() == 15)
        {
            //LOG("CREATING COLLIDERS\n");
            for (pugi::xml_node secondNode = colNode.child("object"); secondNode && ret; secondNode = secondNode.next_sibling("object"))
            {
                //Load the attributes
                object.x = secondNode.next_sibling("object").attribute("x").as_int();
                object.y = secondNode.next_sibling("object").attribute("y").as_int();
                object.width = secondNode.next_sibling("object").attribute("width").as_int();
                object.height = secondNode.next_sibling("object").attribute("height").as_int();
                PhysBody* c1 = app->physics->CreateRectangle(object.x + object.width / 2, object.y + object.height / 2, object.width, object.height, STATIC);
                c1->ctype = ColliderType::PLATFORM;
                Colliders.Add(c1);
            }
        }
        //GROUND SENSOR
        if (colNode.attribute("id").as_int() == 16)
        {
            //LOG("CREATING COLLIDERS\n");
            for (pugi::xml_node secondNode = colNode.child("object"); secondNode && ret; secondNode = secondNode.next_sibling("object"))
            {
                //Load the attributes
                object.x = secondNode.next_sibling("object").attribute("x").as_int();
                object.y = secondNode.next_sibling("object").attribute("y").as_int();
                object.width = secondNode.next_sibling("object").attribute("width").as_int();
                object.height = secondNode.next_sibling("object").attribute("height").as_int();
                PhysBody* c1 = app->physics->CreateRectangleSensor(object.x + object.width / 2, object.y + object.height / 2, object.width, object.height, STATIC);
                c1->ctype = ColliderType::GROUNDSENSOR;
                Colliders.Add(c1);
            }
        }
        //DEATH = 7
        else if (colNode.attribute("id").as_int() == 7)
        {
            for (pugi::xml_node secondNode = colNode.child("object"); secondNode && ret; secondNode = secondNode.next_sibling("object"))
            {
                //Load the attributes
                object.x = secondNode.next_sibling("object").attribute("x").as_int();
                object.y = secondNode.next_sibling("object").attribute("y").as_int();
                object.width = secondNode.next_sibling("object").attribute("width").as_int();
                object.height = secondNode.next_sibling("object").attribute("height").as_int();
                PhysBody* c1 = app->physics->CreateRectangle(object.x + object.width / 2, object.y + object.height / 2, object.width, object.height, STATIC);
                c1->ctype = ColliderType::DEATH;
                Colliders.Add(c1);
            }
        }
        //WIN = 14
        else if (colNode.attribute("id").as_int() == 14)
        {
            for (pugi::xml_node secondNode = colNode.child("object"); secondNode && ret; secondNode = secondNode.next_sibling("object"))
            {
                //Load the attributes
                object.x = secondNode.next_sibling("object").attribute("x").as_int();
                object.y = secondNode.next_sibling("object").attribute("y").as_int();
                object.width = secondNode.next_sibling("object").attribute("width").as_int();
                object.height = secondNode.next_sibling("object").attribute("height").as_int();
                PhysBody* c1 = app->physics->CreateRectangle(object.x + object.width / 2, object.y + object.height / 2, object.width, object.height, STATIC);
                c1->ctype = ColliderType::WINSENSOR;
                Colliders.Add(c1);
            }
        }
        //PLATFORM LIMIT = 21
        else if (colNode.attribute("id").as_int() == 21)
        {
            for (pugi::xml_node secondNode = colNode.child("object"); secondNode && ret; secondNode = secondNode.next_sibling("object"))
            {
                //Load the attributes
                object.x = secondNode.next_sibling("object").attribute("x").as_int();
                object.y = secondNode.next_sibling("object").attribute("y").as_int();
                object.width = secondNode.next_sibling("object").attribute("width").as_int();
                object.height = secondNode.next_sibling("object").attribute("height").as_int();
                PhysBody* c1 = app->physics->CreateRectangleSensor(object.x + object.width / 2, object.y + object.height / 2, object.width, object.height, STATIC);
                c1->ctype = ColliderType::PLATFORMLIMIT;
                Colliders.Add(c1);
            }
        }
    }

    return ret;
}

// L06: DONE 7: Ask for the value of a custom property
Properties::Property* Properties::GetProperty(const char* name)
{
    ListItem<Property*>* item = list.start;
    Property* p = NULL;

    while (item)
    {
        if (item->data->name == name) {
            p = item->data;
            break;
        }
        item = item->next;
    }

    return p;
}


