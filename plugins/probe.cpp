// Just show some position data

#include <iostream>
#include <iomanip>
#include <climits>
#include <vector>
#include <string>
#include <sstream>
#include <ctime>
#include <cstdio>
using namespace std;

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/Units.h"
#include "modules/Maps.h"
#include "modules/Gui.h"
#include "modules/Materials.h"
#include "modules/MapCache.h"
#include "MiscUtils.h"

using std::vector;
using std::string;
using namespace DFHack;

DFhackCExport command_result df_probe (Core * c, vector <string> & parameters);
DFhackCExport command_result df_cprobe (Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "probe";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("probe",
                                     "A tile probe",
                                     df_probe));
    commands.push_back(PluginCommand("cprobe",
                                     "A creature probe",
                                     df_cprobe));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

DFhackCExport command_result df_cprobe (Core * c, vector <string> & parameters)
{
    Console & con = c->con;
    c->Suspend();
    DFHack::Gui *Gui = c->getGui();
    DFHack::Units * cr = c->getUnits();
    int32_t cursorX, cursorY, cursorZ;
    Gui->getCursorCoords(cursorX,cursorY,cursorZ);
    if(cursorX == -30000)
    {
        con.printerr("No cursor; place cursor over creature to probe.\n");
    }
    else
    {
        uint32_t ncr;
        cr->Start(ncr);
        for(auto i = 0; i < ncr; i++)
        {
            df_unit * unit = cr->GetCreature( i );
            if(unit->x == cursorX && unit->y == cursorY && unit->z == cursorZ)
            {
                con.print("Creature %d, race %d (%x), civ %d (%x)\n", unit->id, unit->race, unit->race, unit->civ, unit->civ);
                break;
            }
        }
    }
    c->Resume();
    return CR_OK;
}

DFhackCExport command_result df_probe (Core * c, vector <string> & parameters)
{
    //bool showBlock, showDesig, showOccup, showTile, showMisc;
    Console & con = c->con;
    /*
    if (!parseOptions(parameters, showBlock, showDesig, showOccup,
                      showTile, showMisc))
    {
        con.printerr("Unknown parameters!\n");
        return CR_FAILURE;
    }
    */

    BEGIN_PROBE:
    c->Suspend();

    DFHack::Gui *Gui = c->getGui();
    DFHack::Materials *Materials = c->getMaterials();
    DFHack::VersionInfo* mem = c->vinfo;
    DFHack::Maps *Maps = c->getMaps();
    std::vector<t_matglossInorganic> inorganic;
    bool hasmats = Materials->CopyInorganicMaterials(inorganic);

    if(!Maps->Start())
    {
        con.printerr("Unable to access map data!\n");
    }
    else
    {
        MapExtras::MapCache mc (Maps);

        int32_t regionX, regionY, regionZ;
        Maps->getPosition(regionX,regionY,regionZ);

        bool have_features = Maps->StartFeatures();

        int32_t cursorX, cursorY, cursorZ;
        Gui->getCursorCoords(cursorX,cursorY,cursorZ);
        if(cursorX == -30000)
        {
            con.printerr("No cursor; place cursor over tile to probe.\n");
        }
        else
        {
            DFCoord cursor (cursorX,cursorY,cursorZ);

            uint32_t blockX = cursorX / 16;
            uint32_t tileX = cursorX % 16;
            uint32_t blockY = cursorY / 16;
            uint32_t tileY = cursorY % 16;

            MapExtras::Block * b = mc.BlockAt(cursor/16);
            mapblock40d & block = b->raw;
            if(b && b->valid)
            {
                con.print("block addr: 0x%x\n\n", block.origin);
/*
                if (showBlock)
                {
                    con.print("block flags:\n");
                    print_bits<uint32_t>(block.blockflags.whole,con);
                    con.print("\n\n");
                }
*/
                int16_t tiletype = mc.tiletypeAt(cursor);
                df::tile_designation &des = block.designation[tileX][tileY];
/*
                if(showDesig)
                {
                    con.print("designation\n");
                    print_bits<uint32_t>(block.designation[tileX][tileY].whole,
                                         con);
                    con.print("\n\n");
                }

                if(showOccup)
                {
                    con.print("occupancy\n");
                    print_bits<uint32_t>(block.occupancy[tileX][tileY].whole,
                                         con);
                    con.print("\n\n");
                }
*/

                // tiletype
                con.print("tiletype: %d", tiletype);
                if(tileName(tiletype))
                    con.print(" = %s",tileName(tiletype));
                con.print("\n");

                DFHack::TileShape shape = tileShape(tiletype);
                DFHack::TileMaterial material = tileMaterial(tiletype);
                DFHack::TileSpecial special = tileSpecial(tiletype);
                con.print("%-10s: %4d %s\n","Class"    ,shape,
                       TileShapeString[ shape ]);
                con.print("%-10s: %4d %s\n","Material" ,
                       material,TileMaterialString[ material ]);
                con.print("%-10s: %4d %s\n","Special"  ,
                       special, TileSpecialString[ special ]);
                con.print("%-10s: %4d\n"   ,"Variant"  ,
                       tileVariant(tiletype));
                con.print("%-10s: %s\n"    ,"Direction",
                       tileDirection(tiletype).getStr());
                con.print("\n");

                con.print("temperature1: %d U\n",mc.temperature1At(cursor));
                con.print("temperature2: %d U\n",mc.temperature2At(cursor));

                // biome, geolayer
                con << "biome: " << des.bits.biome << std::endl;
                con << "geolayer: " << des.bits.geolayer_index
                    << std::endl;
                int16_t base_rock = mc.baseMaterialAt(cursor);
                if(base_rock != -1)
                {
                    con << "Layer material: " << dec << base_rock;
                    if(hasmats)
                        con << " / " << inorganic[base_rock].id
                            << " / "
                            << inorganic[base_rock].name
                            << endl;
                    else
                        con << endl;
                }
                int16_t vein_rock = mc.veinMaterialAt(cursor);
                if(vein_rock != -1)
                {
                    con << "Vein material (final): " << dec << vein_rock;
                    if(hasmats)
                        con << " / " << inorganic[vein_rock].id
                            << " / "
                            << inorganic[vein_rock].name
                            << endl;
                    else
                        con << endl;
                }
                // liquids
                if(des.bits.flow_size)
                {
                    if(des.bits.liquid_type == df::tile_liquid::Magma)
                        con <<"magma: ";
                    else con <<"water: ";
                    con << des.bits.flow_size << std::endl;
                }
                if(des.bits.flow_forbid)
                    con << "flow forbid" << std::endl;
                if(des.bits.pile)
                    con << "stockpile?" << std::endl;
                if(des.bits.rained)
                    con << "rained?" << std::endl;
                if(des.bits.smooth)
                    con << "smooth?" << std::endl;
                if(des.bits.water_salt)
                    con << "salty" << endl;
                if(des.bits.water_stagnant)
                    con << "stagnant" << endl;

                #define PRINT_FLAG( X )  con.print("%-16s= %c\n", #X , ( des.X ? 'Y' : ' ' ) )
                PRINT_FLAG( bits.hidden );
                PRINT_FLAG( bits.light );
                PRINT_FLAG( bits.outside );
                PRINT_FLAG( bits.subterranean );
                PRINT_FLAG( bits.water_table );
                PRINT_FLAG( bits.rained );

                DFCoord pc(blockX, blockY);

                if(have_features)
                {
                    t_feature * local = 0;
                    t_feature * global = 0;
                    Maps->ReadFeatures(&(b->raw),&local,&global);
                    PRINT_FLAG( bits.feature_local );
                    if(local)
                    {
                        con.print("%-16s", "");
                        con.print("  %4d", block.local_feature);
                        con.print(" (%2d)", local->type);
                        con.print(" addr 0x%X ", local->origin);
                        con.print(" %s\n", sa_feature(local->type));
                    }
                    PRINT_FLAG( bits.feature_global );
                    if(global)
                    {
                        con.print("%-16s", "");
                        con.print("  %4d", block.global_feature);
                        con.print(" (%2d)", global->type);
                        con.print(" %s\n", sa_feature(global->type));
                    }
                }
                else
                {
                    PRINT_FLAG( bits.feature_local );
                    PRINT_FLAG( bits.feature_global );
                }
                #undef PRINT_FLAG
                con << "local feature idx: " << block.local_feature
                    << endl;
                con << "global feature idx: " << block.global_feature
                    << endl;
                con << "mystery: " << block.mystery << endl;
                con << std::endl;
            }
            else
            {
                con.printerr("No data.\n");
            }
        }
    }
    c->Resume();
    return CR_OK;
}
