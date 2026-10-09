/* App-local indexed actions. Transport never reads authored state. */
#include "control_internal.h"
#include "drawing_program/drawing_program_indexed_editor.h"
#include "drawing_program/drawing_program_indexed_tileset_export.h"
#include "drawing_program/drawing_program_snapshot.h"
#include "drawing_program/drawing_program_project_state.h"
#include <json-c/json.h>
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static json_object *field(json_object *o, const char *key) {
    json_object *v = NULL; (void)json_object_object_get_ex(o, key, &v); return v;
}
static const char *string(json_object *o, const char *key) {
    json_object *v = field(o,key);
    return v && json_object_is_type(v,json_type_string) &&
        strlen(json_object_get_string(v)) == (size_t)json_object_get_string_len(v) ? json_object_get_string(v) : NULL;
}
static json_object *result(const char *code) {
    json_object *o=json_object_new_object();
    json_object_object_add(o,"ok",json_object_new_boolean(!strcmp(code,"ok")));
    json_object_object_add(o,"code",json_object_new_string(code)); return o;
}
static void add_string(json_object *o,const char *key,const char *value) {
    json_object_object_add(o,key,json_object_new_string(value));
}
static uint64_t hash_bytes(uint64_t h,const void *p,size_t n) {
    const unsigned char *b=p; while(n--) h=(h^*b++)*UINT64_C(1099511628211); return h;
}
static void state_token(DrawingProgramAppContext *app,char out[17]) {
    DrawingProgramTextureProject *p=&app->texture_project;
    uint64_t h=UINT64_C(1469598103934665603);
    h=hash_bytes(h,&app->session.control_document_generation,sizeof(uint64_t));
    h=hash_bytes(h,&p->profile_kind,sizeof(p->profile_kind));
    h=hash_bytes(h,&p->indexed_profile,sizeof(p->indexed_profile));
    h=hash_bytes(h,&p->indexed_cells,sizeof(p->indexed_cells));
    if(p->indexed_raster.indices) h=hash_bytes(h,p->indexed_raster.indices,p->indexed_raster.index_count);
    h=hash_bytes(h,&p->indexed_history.command_count,sizeof(uint32_t));
    h=hash_bytes(h,&p->indexed_history.command_cursor,sizeof(uint32_t));
    h=hash_bytes(h,&p->indexed_cell_history.command_count,sizeof(uint32_t));
    h=hash_bytes(h,&p->indexed_cell_history.command_cursor,sizeof(uint32_t));
    snprintf(out,17,"%016llx",(unsigned long long)h);
}
static int integer(json_object *v,int min,int max,int *out) {
    if(!v || !json_object_is_type(v,json_type_int)) return 0;
    int64_t i=json_object_get_int64(v); if(i<min || i>max) return 0; *out=(int)i; return 1;
}
static const DrawingProgramIndexedCell *find_cell(DrawingProgramAppContext *app,json_object *req,const char *key) {
    const char *id=string(req,key);
    return id ? drawing_program_indexed_cell_table_find(&app->texture_project.indexed_cells,id) : NULL;
}
static void read_bytes(DrawingProgramAppContext *app,const DrawingProgramIndexedCell *cell,uint8_t out[256]) {
    for(unsigned i=0;i<256;i++) out[i]=app->texture_project.indexed_raster.indices[
        (cell->y+i/16)*app->texture_project.indexed_raster.width+cell->x+i%16];
}
static json_object *tile_read(DrawingProgramAppContext *app,const DrawingProgramIndexedCell *cell) {
    json_object *o=result("ok"), *indices=json_object_new_array(), *rgba=json_object_new_array();
    uint8_t bytes[256]; read_bytes(app,cell,bytes); add_string(o,"tile",cell->id);
    for(unsigned i=0;i<256;i++) {
        json_object_array_add(indices,json_object_new_int(bytes[i]));
        CoreAuthoredTextureRgba8 color=app->texture_project.indexed_profile.slots[bytes[i]].preview_rgba;
        json_object *pixel=json_object_new_array();
        json_object_array_add(pixel,json_object_new_int(color.r)); json_object_array_add(pixel,json_object_new_int(color.g));
        json_object_array_add(pixel,json_object_new_int(color.b)); json_object_array_add(pixel,json_object_new_int(color.a));
        json_object_array_add(rgba,pixel);
    }
    json_object_object_add(o,"indices",indices); json_object_object_add(o,"preview_rgba",rgba); return o;
}
static int safe_name(const char *s) {
    if(!s || !core_authored_texture_identifier_validate(s)) return 0;
    return strcmp(s,".") && strcmp(s,"..");
}
static json_object *write_output(SketchControl *control,DrawingProgramAppContext *app,json_object *req,const char *op) {
    const char *name=string(req,"name"); char current[PATH_MAX],path[640],stage[700],backup[700]; struct stat info;
    if(!safe_name(name) || !realpath(control->output_root,current) || strcmp(current,control->output_root))
        return result("invalid_output");
    snprintf(path,sizeof(path),"%s/%s%s",current,name,!strcmp(op,"save_candidate")?".pack":"");
    if(lstat(path,&info)==0) return result("destination_exists");
    CoreResult r;
    if(!strcmp(op,"save_candidate")) {
        snprintf(stage,sizeof(stage),"%s.stage-%ld",path,(long)getpid());
        if(lstat(stage,&info)==0) return result("staging_exists");
        r=drawing_program_snapshot_save(app,stage);
        if(r.code==CORE_OK && link(stage,path)!=0) r=(CoreResult){CORE_ERR_IO,"candidate commit failed"};
        (void)unlink(stage);
    } else {
        /* Existing exporter owns these exact temporary names; refuse foreign staging. */
        snprintf(stage,sizeof(stage),"%s.dpt-stage-%ld",path,(long)getpid());
        snprintf(backup,sizeof(backup),"%s.dpt-backup-%ld",path,(long)getpid());
        if(lstat(stage,&info)==0 || lstat(backup,&info)==0) return result("staging_exists");
        const char *palette=string(req,"palette");
        DrawingProgramIndexedTilesetExportOptions opts={.palette_id=palette?palette:"default"};
        DrawingProgramIndexedTilesetExportReport report;
        r=drawing_program_indexed_tileset_export(&app->texture_project,path,&opts,&report);
    }
    json_object *o=result(r.code==CORE_OK?"ok":"output_failed");
    if(r.code==CORE_OK) add_string(o,"path",path);
    else add_string(o,"detail",r.message?r.message:"output failed");
    return o;
}
static json_object *edit(DrawingProgramAppContext *app,json_object *req,const char *op) {
    if(!strcmp(op,"undo") || !strcmp(op,"redo")) {
        CoreResult r=!strcmp(op,"undo")?drawing_program_indexed_editor_undo(app):drawing_program_indexed_editor_redo(app);
        return result(r.code==CORE_OK?"ok":"history_unavailable");
    }
    const DrawingProgramIndexedCell *cell=find_cell(app,req,"tile");
    if(!cell || cell->width!=16 || cell->height!=16) return result("unknown_tile");
    uint8_t bytes[256]; read_bytes(app,cell,bytes);
    int slots=(int)app->texture_project.indexed_profile.slot_count;
    if(!strcmp(op,"replace_tile")) {
        json_object *array=field(req,"indices");
        if(!array || !json_object_is_type(array,json_type_array) || json_object_array_length(array)!=256) return result("invalid_indices");
        for(unsigned i=0;i<256;i++) {int v; if(!integer(json_object_array_get_idx(array,i),0,slots-1,&v)) return result("invalid_indices"); bytes[i]=(uint8_t)v;}
    } else if(!strcmp(op,"patch_tile")) {
        json_object *array=field(req,"pixels"); bool seen[256]={0};
        if(!array || !json_object_is_type(array,json_type_array) || json_object_array_length(array)>256) return result("invalid_patch");
        for(unsigned i=0;i<json_object_array_length(array);i++) {
            json_object *pixel=json_object_array_get_idx(array,i); int x,y,v;
            if(!pixel || !json_object_is_type(pixel,json_type_object) || json_object_object_length(pixel)!=3 ||
                !integer(field(pixel,"x"),0,15,&x) || !integer(field(pixel,"y"),0,15,&y) || !integer(field(pixel,"slot"),0,slots-1,&v) || seen[y*16+x]) return result("invalid_patch");
            seen[y*16+x]=true; bytes[y*16+x]=(uint8_t)v;
        }
    } else if(!strcmp(op,"copy_tile")) {
        const DrawingProgramIndexedCell *source=find_cell(app,req,"source_tile");
        if(!source || source->width!=16 || source->height!=16) return result("unknown_source_tile");
        read_bytes(app,source,bytes);
    } else if(!strcmp(op,"fill_tile")) {
        int x,y,v; if(!integer(field(req,"x"),0,15,&x) || !integer(field(req,"y"),0,15,&y) || !integer(field(req,"slot"),0,slots-1,&v)) return result("invalid_fill");
        uint8_t previous=app->ui.indexed_selected_slot; app->ui.indexed_selected_slot=(uint8_t)v;
        CoreResult r=drawing_program_indexed_editor_apply_at(app,DRAWING_PROGRAM_TOOL_FILL,cell->x+(unsigned)x,cell->y+(unsigned)y);
        app->ui.indexed_selected_slot=previous; return result(r.code==CORE_OK?"ok":"edit_failed");
    } else return result("unknown_operation");
    CoreResult r=drawing_program_indexed_editor_replace_tile(app,cell->id,bytes);
    return result(r.code==CORE_OK?"ok":"edit_failed");
}
static int mutation(const char *op) {
    return !strcmp(op,"replace_tile") || !strcmp(op,"patch_tile") || !strcmp(op,"fill_tile") ||
        !strcmp(op,"copy_tile") || !strcmp(op,"undo") || !strcmp(op,"redo") || !strcmp(op,"save_candidate") || !strcmp(op,"export_candidate");
}
static int allowed_fields(json_object *req,const char *op) {
    const char *extra=!strcmp(op,"replace_tile")?"tile indices":!strcmp(op,"patch_tile")?"tile pixels":
        !strcmp(op,"fill_tile")?"tile x y slot":!strcmp(op,"copy_tile")?"tile source_tile":
        !strcmp(op,"save_candidate")?"name":!strcmp(op,"export_candidate")?"name palette":
        !strcmp(op,"read_tile")?"tile":!strcmp(op,"request_status")?"instance request_id":"";
    json_object_object_foreach(req,key,value) {
        (void)value;
        if(!strcmp(key,"op") || (mutation(op) && (!strcmp(key,"instance") || !strcmp(key,"request_id") || !strcmp(key,"expected_state")))) continue;
        char token[100]; snprintf(token,sizeof(token),"%s",extra); bool found=false;
        char *context=NULL;
        for(char *part=strtok_r(token," ",&context);part;part=strtok_r(NULL," ",&context)) if(!strcmp(part,key)) found=true;
        if(!found) return 0;
    }
    return 1;
}
char *sketch_control_dispatch(SketchControl *control,DrawingProgramAppContext *app,const char *raw,bool busy,bool *changed) {
    *changed=false; json_object *req=NULL,*o=NULL; int receipt=-1;
    json_tokener *tok=json_tokener_new(); req=json_tokener_parse_ex(tok,raw,(int)strlen(raw));
    size_t end=json_tokener_get_parse_end(tok); bool parsed=json_tokener_get_error(tok)==json_tokener_success;
    json_tokener_free(tok); while(raw[end]==' ' || raw[end]=='\t' || raw[end]=='\r') end++;
    const char *op=req && json_object_is_type(req,json_type_object)?string(req,"op"):NULL;
    if(!parsed || !op || raw[end] || !allowed_fields(req,op)) {o=result("invalid_request"); goto done;}
    const char *instance=string(req,"instance"),*id=string(req,"request_id");
    if(mutation(op) || !strcmp(op,"request_status")) {
        if(!instance || strcmp(instance,control->instance)) {o=result("stale_instance"); goto done;}
        if(!id || !*id || strlen(id)>64) {o=result("invalid_request_id"); goto done;}
        for(int i=0;i<control->receipt_count;i++) if(!strcmp(id,control->receipts[i].id)) {
            if(strcmp(op,"request_status") && strcmp(raw,control->receipts[i].request)) {o=result("request_id_conflict"); goto done;}
            char *replay=strdup(control->receipts[i].response); json_object_put(req); return replay;
        }
        if(!strcmp(op,"request_status")) {o=result("unknown_request"); goto done;}
        if(control->receipt_count==SKETCH_CONTROL_RECEIPTS) {o=result("receipt_capacity"); goto done;}
        char *copy=strdup(raw),*fallback=strdup("{\"ok\":false,\"code\":\"allocation_failed\",\"may_have_applied\":true}");
        if(!copy || !fallback) {free(copy);free(fallback);o=result("allocation_failed");goto done;}
        receipt=control->receipt_count++; SketchReceipt *r=&control->receipts[receipt];
        strcpy(r->id,id);r->request=copy;r->response=fallback;
        char state[17]; state_token(app,state);
        if(!control->allow_write) {o=result("read_only");goto done;}
        const char *expected=string(req,"expected_state");
        if(!expected || strcmp(state,expected)) {o=result("stale_state");goto done;}
        if(busy) {o=result("busy");goto done;}
        if(!drawing_program_indexed_editor_is_active(app)) {o=result("unsupported_profile");goto done;}
        o=(!strcmp(op,"save_candidate") || !strcmp(op,"export_candidate"))?write_output(control,app,req,op):edit(app,req,op);
        char after[17];state_token(app,after);*changed=strcmp(state,after)!=0;
        json_object_object_add(o,"changed",json_object_new_boolean(*changed));goto done;
    }
    if(!strcmp(op,"capabilities")) {
        o=result("ok");add_string(o,"instance",control->instance);
        json_object_object_add(o,"write_enabled",json_object_new_boolean(control->allow_write));
        json_object *ops=json_object_new_array();
        const char *names[]={"inspect_project","read_tile","replace_tile","patch_tile","fill_tile","copy_tile","undo","redo","save_candidate","export_candidate","request_status"};
        for(unsigned i=0;i<sizeof(names)/sizeof(names[0]);i++) json_object_array_add(ops,json_object_new_string(names[i]));
        json_object_object_add(o,"operations",ops);
    } else if(!strcmp(op,"inspect_project")) {
        o=result("ok");bool active=drawing_program_indexed_editor_is_active(app);
        add_string(o,"profile",active?"INDEXED_ATLAS_V1":"STANDARD_RGBA");
        json_object_object_add(o,"dirty",json_object_new_boolean(drawing_program_project_state_current_is_dirty(app)));
        json_object *tiles=json_object_new_array(),*slots=json_object_new_array();
        if(active) {
            add_string(o,"tileset",app->texture_project.indexed_profile.tileset_id);
            for(unsigned i=0;i<app->texture_project.indexed_cells.count;i++) json_object_array_add(tiles,json_object_new_string(app->texture_project.indexed_cells.cells[i].id));
            for(unsigned i=0;i<app->texture_project.indexed_profile.slot_count;i++) json_object_array_add(slots,json_object_new_string(app->texture_project.indexed_profile.slots[i].id));
        }
        json_object_object_add(o,"tiles",tiles);json_object_object_add(o,"slots",slots);
    } else if(!strcmp(op,"read_tile")) {
        if(!drawing_program_indexed_editor_is_active(app)) o=result("unsupported_profile");
        else {const DrawingProgramIndexedCell *cell=find_cell(app,req,"tile");o=cell && cell->width==16 && cell->height==16?tile_read(app,cell):result("unknown_tile");}
    } else o=result("unknown_operation");
done:
    if(o) {char state[17];state_token(app,state);add_string(o,"expected_state",state);add_string(o,"instance",control->instance);}
    char *response=o?strdup(json_object_to_json_string_ext(o,JSON_C_TO_STRING_PLAIN)):NULL;
    if(receipt>=0 && response) {char *retained=strdup(response);if(retained) {free(control->receipts[receipt].response);control->receipts[receipt].response=retained;}}
    if(o) json_object_put(o);if(req) json_object_put(req);return response;
}
