#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "4coder_default_include.cpp"

#define _XOPEN_SOURCE 700
#include <sys/types.h>
#include <dirent.h>

#define internal static

enum {
    PROJ_NOT = 0,
    PROJ_HAS_PROJECT = 1,
    PROJ_HAS_MAKEFILE = 2,
    PROJ_HAS_BUILDSH = 4,
};

internal int is_4coder_project(const char* path) {
    DIR* dir;
    dirent* ent;
    int ret = PROJ_NOT;
    
    dir = opendir(path);
    
    if(dir) {
        while((ent = readdir(dir)) && !ret) {
            if(ent->d_type == DT_REG) {
                if(strcmp("project.4coder", ent->d_name) == 0) {
                    ret |= PROJ_HAS_PROJECT;
                }
                if(strcmp("Makefile", ent->d_name) == 0) {
                    ret |= PROJ_HAS_MAKEFILE;
                }
                if(strcmp("build.sh", ent->d_name) == 0 || strcmp("build.bat", ent->d_name) == 0) {
                    ret |= PROJ_HAS_BUILDSH;
                }
            }
        }
        closedir(dir);
    }
    return ret;
}

struct easi_project {
    char* path;
    int path_len;
    char* name;
    int name_len;
    
    int type;
    
    char status[4];
    
    ~easi_project() {
        if(path) {
            delete[] path;
        }
        if(name) {
            delete[] name;
        }
    }
};

easi_project* proj_cache = nullptr;

internal void easi_load_proj(Application_Links *app, Partition *scratch, Heap *heap,
                             View_Summary *view, Lister_State *state,
                             String text_field, void *user_data, bool32 activated_by_mouse){
    easi_project* proj = (easi_project*)user_data;
    
    save_all_dirty_buffers(app);
    directory_set_hot(app, proj->path, proj->path_len);
    if(proj->type & PROJ_HAS_PROJECT) {
        set_current_project_from_nearest_project_file(app, &global_part);
    }
    
    lister_default(app, scratch, heap, view, state, ListerActivation_Finished);
    
    delete[] proj_cache;
}

CUSTOM_COMMAND_SIG(easi_list_projects)
CUSTOM_DOC("[EASI] Lists projects")
{
    DIR* dir;
    dirent* ent;
    dir = opendir("/home/easimer/projects/");
    
    if(!dir) {
        print_message(app, "Failed to open /home/easimer/projects!\n", 0);
        return;
    }
    
    Partition *arena = &global_part;
    Temp_Memory temp = begin_temp_memory(arena);
    
    int project_count = 0;
    easi_project* projects = nullptr;
    
    while(ent = readdir(dir)) {
        if(ent->d_type == DT_DIR) {
            char abs_path[1024];
            // TODO(easimer): secure this
            snprintf(abs_path, 1024, "%s/%s/", "/home/easimer/projects/", ent->d_name);
            if(is_4coder_project(abs_path)) {
                project_count++;
            }
        }
    }
    
    rewinddir(dir);
    
    projects = new easi_project[project_count];
    int idx = 0;
    
    while(ent = readdir(dir)) {
        if(ent->d_type == DT_DIR) {
            char abs_path[1024];
            // TODO(easimer): secure this
            snprintf(abs_path, 1024, "%s/%s/", "/home/easimer/projects/", ent->d_name);
            int type = is_4coder_project(abs_path);
            if(type != PROJ_NOT) {
                projects[idx].path_len = strlen(abs_path);
                projects[idx].path = new char[projects[idx].path_len + 1];
                strncpy(projects[idx].path, abs_path, projects[idx].path_len);
                projects[idx].name_len = strlen(ent->d_name);
                projects[idx].name = new char[projects[idx].name_len + 1];
                strncpy(projects[idx].name, ent->d_name, projects[idx].name_len);
                projects[idx].type = type;
                projects[idx].status[0] = (projects[idx].type & PROJ_HAS_PROJECT) ? '4' : '_';
                projects[idx].status[1] = (projects[idx].type & PROJ_HAS_MAKEFILE) ? 'M' : '_';
                projects[idx].status[2] = (projects[idx].type & PROJ_HAS_BUILDSH) ? 'B' : '_';
                projects[idx].status[3] = 0;
                idx++;
            }
        }
    }
    
    View_Summary view = get_active_view(app, AccessAll);
    view_end_ui_mode(app, &view);
    
    Lister_Option* options = push_array(arena, Lister_Option, project_count);
    for(int i = 0; i < project_count; i++) {
        options[i].string = make_string(projects[i].name, projects[i].name_len);
        char status[4] = {0};
        options[i].status = make_string(projects[i].status, 3);
        options[i].user_data = projects + i;
    }
    
    proj_cache = projects;
    
    begin_integrated_lister__basic_list(app, "Projects:", easi_load_proj, 0, 0,
                                        options, project_count,
                                        0,
                                        &view);
    
    end_temp_memory(temp);
}

START_HOOK_SIG(easi_start)
{
    default_4coder_initialize(app);
    return(0);
}

extern "C" GET_BINDING_DATA(get_bindings)
{
    Bind_Helper context_actual = begin_bind_helper(data, size);
    Bind_Helper *context = &context_actual;
    
    set_start_hook(context, easi_start);
    set_command_caller(context, default_command_caller);
    
    begin_map(context, mapid_global);
    bind(context, 'p', MDFR_ALT, easi_list_projects); // alt-p - list projects
    end_map(context);
    
    set_all_default_hooks(context);
    default_keys(context);
    end_bind_helper(context);
    return context->write_total;
}