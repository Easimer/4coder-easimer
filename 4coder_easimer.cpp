#ifndef FCODER_DEFAULT_BINDINGS_CPP
#define FCODER_DEFAULT_BINDINGS_CPP

#include "4coder_default_include.cpp"

#if !defined(META_PASS)
#include "generated/managed_id_metadata.cpp"
#endif

#include "libstuff.cpp"

void custom_layer_init(Application_Links* app) {
    auto tctx = get_thread_context(app);
    
    // NOTE(allen): setup for default framework
    default_framework_init(app);
    
    // NOTE(allen): default hooks and command maps
    set_all_default_hooks(app);
    mapping_init(tctx, &framework_mapping);
    setup_default_mapping(&framework_mapping, mapid_global, mapid_file, mapid_code);
}

#endif /* FCODER_DEFAULT_BINDINGS_CPP */