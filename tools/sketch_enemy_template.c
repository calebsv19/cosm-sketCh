/* Empty, named top-down enemy carrier. Pixel art is authored through MCP. */
#include "../src/io/session/drawing_program_texture_project_snapshot_internal.h"
#include "drawing_program/drawing_program_texture_project.h"
#include "drawing_program/drawing_program_indexed_cells.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
int main(int argc,char **argv) {
    if((argc!=2 && !(argc==3&&(!strcmp(argv[2],"--objects")||!strcmp(argv[2],"--effects")||!strcmp(argv[2],"--weapons")))) || access(argv[1],F_OK)==0) return 2;
    int weapons=argc==3&&!strcmp(argv[2],"--weapons");
    int objects=argc==3&&!strcmp(argv[2],"--objects"),effects=argc==3&&!strcmp(argv[2],"--effects");
    static DrawingProgramDocument doc;static DrawingProgramLayerRasterStore layers;
    DrawingProgramTextureProject *project=calloc(1,sizeof(*project));if(!project)return 1;static DrawingProgramIndexedTilesetProfile p;static CorePackWriter writer;
    const char *enemy_names[]={"enemy.rat.idle","enemy.spider.idle","enemy.goblin.idle","enemy.swordsman.idle","enemy.gunman.idle","enemy.wizard.idle","weapon.sword.steel","weapon.sword.ice","weapon.gun.pistol","weapon.gun.shotgun","weapon.staff.ember"};
    const char *object_names[]={"chest.wood.closed","chest.wood.locked","chest.wood.open","player.body.idle","weapon.sword.wood","weapon.sword.fire"};
    const char *effect_names[]={"camp.unlit","camp.lit.0","camp.lit.1","camp.lit.2","projectile.bullet","projectile.fire.0","projectile.fire.1","projectile.ice","projectile.poison","projectile.arcane","projectile.heal","projectile.lightning","effect.impact.0","effect.impact.1","effect.impact.2","effect.damage.0","effect.damage.1","effect.damage.2","effect.aura.heal","effect.aura.poison","effect.aura.fire","effect.aura.ice","effect.aura.arcane","effect.aura.lightning","effect.aura.shield","effect.aura.speed","projectile.enemy_bullet"};
    const char *weapon_names[]={"weapon.sword.wood","weapon.sword.iron","weapon.sword.steel","weapon.sword.heavy","weapon.sword.long","weapon.sword.fire","weapon.sword.ice","weapon.sword.freeze","weapon.sword.rage","weapon.sword.poison","weapon.sword.weakness","weapon.sword.blind","weapon.sword.dual_spin","weapon.sword.dual","weapon.sword.spinner","weapon.gun.pistol","weapon.gun.heavy","weapon.gun.semi","weapon.gun.shotgun","weapon.gun.wide","weapon.gun.tight","weapon.gun.machine","weapon.gun.tracking","weapon.gun.surround","cue.muzzle.0","cue.muzzle.1","cue.cast.0","cue.cast.1"};
    const char **names=weapons?weapon_names:effects?effect_names:objects?object_names:enemy_names;unsigned count=weapons?28:effects?27:objects?6:11,width=count*16;
    const char *slots[]={"transparent","outline","shadow","fur","fur_light","tail","green","green_light","leather","steel","steel_light","cloth_red","cloth_blue","cloth_light","skin","wood","gold","magic","player_teal","player_light","fire_orange","fire_light","ice_blue","poison_green"};
    const unsigned char rgb[][3]={{0,0,0},{20,23,30},{43,46,55},{76,73,69},{111,106,96},{106,88,86},{86,124,62},{135,166,83},{94,64,48},{111,132,147},{191,209,211},{137,61,55},{71,84,145},{119,143,194},{207,164,119},{127,94,56},{216,181,85},{102,219,202},{48,146,166},{102,190,196},{211,100,45},{242,191,105},{136,195,212},{116,167,75}};
    p.contract_revision=1;p.atlas_width=width;p.atlas_height=16;p.logical_cell_width=p.logical_cell_height=16;p.slot_count=(effects||weapons)?24:objects?20:18;
    snprintf(p.tileset_id,sizeof(p.tileset_id),"%s",weapons?"weapon_pixels_v1":effects?"combat_pixels_v1":objects?"game_objects_v1":"enemy_topdown_v1");
    for(unsigned i=0;i<p.slot_count;++i){snprintf(p.slots[i].id,sizeof(p.slots[i].id),"%s",slots[i]);p.slots[i].source_rgba=(CoreAuthoredTextureRgba8){rgb[i][0],rgb[i][1],rgb[i][2],i?255:0};p.slots[i].preview_rgba=p.slots[i].source_rgba;}
    int ok=drawing_program_document_init_with_shape(&doc,width,16,1).code==CORE_OK &&
        drawing_program_layer_raster_store_init_from_document(&layers,&doc).code==CORE_OK &&
        drawing_program_texture_project_init_single_surface(project,&doc,&layers,"Top-down enemies",DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_STANDARD).code==CORE_OK &&
        drawing_program_texture_project_enable_indexed_atlas(project,&p,0).code==CORE_OK;
    for(unsigned i=0;ok && i<count;++i){char id[64];snprintf(id,sizeof(id),"%s",names[i]);ok=drawing_program_indexed_cell_create(&project->indexed_cells,&project->indexed_cell_history,id,i*16,0,16,16).code==CORE_OK;}
    if(ok)ok=core_pack_writer_open(argv[1],&writer).code==CORE_OK;
    if(ok){ok=drawing_program_texture_project_snapshot_write(&writer,project).code==CORE_OK;ok=core_pack_writer_close(&writer).code==CORE_OK&&ok;}
    drawing_program_texture_project_dispose(project);drawing_program_layer_raster_store_dispose(&layers);
    free(project);if(!ok)return 1;printf("Created empty body/weapon indexed template: %s\n",argv[1]);return 0;
}
