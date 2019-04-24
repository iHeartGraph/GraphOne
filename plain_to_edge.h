#pragma once

#include <string>
#include <dirent.h>
#include <assert.h>
#include <string>
#include <unistd.h>

#include "type.h"
#include "graph_view.h"
#include "graph.h"
#include "typekv.h"
#include "sgraph.h"
#include "util.h"

using namespace std;

extern index_t residue;

template <class T>
class plaingraph_manager_t {
  public:
    //Class member, pgraph_t<T> only
    cfinfo_t* pgraph; 
    
  public:
    plaingraph_manager_t() {
        pgraph = 0;
    }

    inline pgraph_t<T>* get_plaingraph() {
        return static_cast<pgraph_t<T>*> (pgraph);
    }
    
    inline void
    set_pgraph(cfinfo_t* a_pgraph) {
        pgraph = a_pgraph;
    }

    void schema_plaingraph();
    void schema_plaingraphd();
    void schema_plaingraphuni();

  public:
    void setup_graph(vid_t v_count);
    void setup_graph_memory(vid_t v_count);
    void setup_graph_vert_nocreate(vid_t v_count);
    void recover_graph_adj(const string& idirname, const string& odirname);

    void prep_graph_fromtext(const string& idirname, const string& odirname, 
                             typename callback<T>::parse_fn_t);
    void prep_graph_fromtext2(const string& idirname, const string& odirname, 
                              typename callback<T>::parse_fn2_t parsebuf_fn);
    void prep_graph_edgelog_fromtext(const string& idirname, 
        const string& odirname, typename callback<T>::parse_fn2_t parsefile_fn);

    void prep_graph_edgelog(const string& idirname, const string& odirname);
    void prep_graph_adj(const string& idirname, const string& odirname);
    void prep_graph_mix(const string& idirname, const string& odirname);
    void prep_graph(const string& idirname, const string& odirname);
    void prep_graph_durable(const string& idirname, const string& odirname);
    
    void prep_graph_serial_scompute(const string& idirname, const string& odirname, sstream_t<T>* sstreamh);
    void prep_graph_and_scompute(const string& idirname, const string& odirname,
                                 sstream_t<T>* sstreamh);
    void prep_graph_and_compute(const string& idirname, 
                                const string& odirname, 
                                stream_t<T>* streamh);
    void waitfor_archive_durable(double start);
    
    void run_pr_simple();
    void run_pr();
    void run_prd();
    void run_bfs(sid_t root = 1);
    void run_1hop();
    void run_2hop();
};

template <class T>
void plaingraph_manager_t<T>::schema_plaingraph()
{
    g->cf_info = new cfinfo_t*[2];
    g->p_info = new pinfo_t[2];
    
    pinfo_t*    p_info    = g->p_info;
    cfinfo_t*   info      = 0;
    const char* longname  = 0;
    const char* shortname = 0;
    
    longname = "gtype";
    shortname = "gtype";
    info = new typekv_t;
    g->add_columnfamily(info);
    info->add_column(p_info, longname, shortname);
    ++p_info;
    
    longname = "friend";
    shortname = "friend";
    info = new ugraph<T>;
    g->add_columnfamily(info);
    info->add_column(p_info, longname, shortname);
    info->flag1 = 1;
    info->flag2 = 1;
    ++p_info;
    set_pgraph(info);

}

template <class T>
void plaingraph_manager_t<T>::schema_plaingraphd()
{
    g->cf_info = new cfinfo_t*[2];
    g->p_info = new pinfo_t[2];
    
    pinfo_t*    p_info    = g->p_info;
    cfinfo_t*   info      = 0;
    const char* longname  = 0;
    const char* shortname = 0;
    
    longname = "gtype";
    shortname = "gtype";
    info = new typekv_t;
    g->add_columnfamily(info);
    info->add_column(p_info, longname, shortname);
    ++p_info;
    
    longname = "friend";
    shortname = "friend";
    info = new dgraph<T>;
    g->add_columnfamily(info);
    info->add_column(p_info, longname, shortname);
    info->flag1 = 1;
    info->flag2 = 1;
    ++p_info;
    set_pgraph(info);
}

template <class T>
void plaingraph_manager_t<T>::schema_plaingraphuni()
{
    g->cf_info = new cfinfo_t*[2];
    g->p_info = new pinfo_t[2];
    
    pinfo_t*    p_info    = g->p_info;
    cfinfo_t*   info      = 0;
    const char* longname  = 0;
    const char* shortname = 0;
    
    longname = "gtype";
    shortname = "gtype";
    info = new typekv_t;
    g->add_columnfamily(info);
    info->add_column(p_info, longname, shortname);
    ++p_info;
    
    longname = "friend";
    shortname = "friend";
    info = new unigraph<T>;
    g->add_columnfamily(info);
    info->add_column(p_info, longname, shortname);
    info->flag1 = 1;
    info->flag2 = 1;
    ++p_info;
    set_pgraph(info);
}

template <class T>
void plaingraph_manager_t<T>::setup_graph(vid_t v_count)
{
    //do some setup for plain graphs
    typekv_t* typekv = g->get_typekv();
    typekv->manual_setup(v_count, true);
    g->type_done();
    g->prep_graph_baseline();
    g->file_open(true);
}

template <class T>
void plaingraph_manager_t<T>::setup_graph_vert_nocreate(vid_t v_count)
{
    //do some setup for plain graphs
    typekv_t* typekv = g->get_typekv();
    typekv->manual_setup(v_count, false);
    g->type_done();
    g->prep_graph_baseline();
    g->file_open(true);
}

template <class T>
void plaingraph_manager_t<T>::setup_graph_memory(vid_t v_count)
{
    //do some setup for plain graphs
    typekv_t* typekv = g->get_typekv();
    typekv->manual_setup(v_count, true);
    //g->file_open(true);
    //g->prep_graph_baseline();
    //g->file_open(true);
}

extern vid_t v_count;

struct arg_t {
    string file;
    void* manager;
};

//void* recovery_func(void* arg); 

template <class T>
void* recovery_func(void* a_arg) 
{
    arg_t* arg = (arg_t*) a_arg; 
    string filename = arg->file;
    plaingraph_manager_t<T>* manager = (plaingraph_manager_t<T>*)arg->manager;

    pgraph_t<T>* ugraph = (pgraph_t<T>*)manager->get_plaingraph();
    blog_t<T>*     blog = ugraph->blog;
    
    index_t to_read = 0;
    index_t total_read = 0;
    index_t batch_size = (1L << residue);
    cout << "batch_size = " << batch_size << endl;
        
    FILE* file = fopen(filename.c_str(), "rb");
    assert(file != 0);
    index_t size = fsize(filename);
    index_t edge_count = size/sizeof(edgeT_t<T>);
    
    //Lets set the edge log higher
    index_t new_count = upper_power_of_two(edge_count);
    ugraph->alloc_edgelog(new_count);
    edgeT_t<T>*    edge = blog->blog_beg;
    cout << "edge_count = " << edge_count << endl;
    cout << "new_count  = " << new_count << endl;
    
    //XXX some changes require to be made if edge log size is finite.   
    while (total_read < edge_count) {
        to_read = min(edge_count - total_read,batch_size);
        if (to_read != fread(edge + total_read, sizeof(edgeT_t<T>), to_read, file)) {
            assert(0);
        }
        blog->blog_head += to_read;
        total_read += to_read;
    }
    return 0;
}

template <class T>
void plaingraph_manager_t<T>::recover_graph_adj(const string& idirname, const string& odirname)
{
    string idir = idirname;
    index_t batch_size = (1L << residue);
    cout << "batch_size = " << batch_size << endl;

    arg_t* arg = new arg_t;
    arg->file = idir;
    arg->manager = this;
    pthread_t recovery_thread;
    if (0 != pthread_create(&recovery_thread, 0, &recovery_func<T> , arg)) {
        assert(0);
    }
    
    pgraph_t<T>* ugraph = (pgraph_t<T>*)get_plaingraph();
    blog_t<T>*     blog = ugraph->blog;
    index_t marker = 0;
    //index_t snap_marker = 0;
    index_t size = fsize(idirname);
    index_t edge_count = size/sizeof(edgeT_t<T>);
    
    double start = mywtime();
    
    //Make Graph
    while (0 == blog->blog_head) {
        usleep(20);
    }
    while (marker < edge_count) {
        usleep(20);
        while (marker < blog->blog_head) {
            marker = min(blog->blog_head, marker+batch_size);
            ugraph->create_marker(marker);
            ugraph->create_snapshot();
        }
    }
    delete arg; 
    double end = mywtime ();
    cout << "Make graph time = " << end - start << endl;
}

template <class T>
void plaingraph_manager_t<T>::prep_graph_adj(const string& idirname, const string& odirname)
{
    pgraph_t<T>* ugraph = (pgraph_t<T>*)get_plaingraph();
    blog_t<T>*     blog = ugraph->blog;
    
    free(blog->blog_beg);
    blog->blog_beg = 0;
    blog->blog_head  += read_idir(idirname, &blog->blog_beg, false);
    
    //Upper align this, and create a mask for it
    index_t new_count = upper_power_of_two(blog->blog_head);
    blog->blog_mask = new_count -1;
    
    double start = mywtime();
    
    //Make Graph
    index_t marker = 0;
    index_t batch_size = (1L << residue);
    cout << "batch_size = " << batch_size << endl;

    while (marker < blog->blog_head) {
        marker = min(blog->blog_head, marker+batch_size);
        ugraph->create_marker(marker);
        ugraph->create_snapshot();
    }
    double end = mywtime ();
    cout << "Make graph time = " << end - start << endl;
}

template <class T>
void plaingraph_manager_t<T>::prep_graph_mix(const string& idirname, const string& odirname)
{
    pgraph_t<T>* ugraph = (pgraph_t<T>*)get_plaingraph();
    blog_t<T>*     blog = ugraph->blog;
    
    free(blog->blog_beg);
    blog->blog_beg = 0;
    blog->blog_head  += read_idir(idirname, &blog->blog_beg, false);
    
    //Upper align this, and create a mask for it
    index_t new_count = upper_power_of_two(blog->blog_head);
    blog->blog_mask = new_count -1;
    
    double start = mywtime();
    
    //Make Graph
    index_t marker = 0;
    index_t batch_size = (1L << residue);
    cout << "edge count in view = " << batch_size << endl;

    marker = blog->blog_head - batch_size;
    ugraph->create_marker(marker);
    ugraph->create_snapshot();
    double end = mywtime ();
    cout << "Make graph time = " << end - start << endl;
}

template <class T>
void plaingraph_manager_t<T>::prep_graph_edgelog(const string& idirname, const string& odirname)
{
    edgeT_t<T>* edges = 0;
    index_t total_edge_count = read_idir(idirname, &edges, true); 
    
    pgraph_t<T>* ugraph = (pgraph_t<T>*)get_plaingraph();
    
    //We need to increase the size of edge log to test logging functionalities
    ugraph->alloc_edgelog(total_edge_count);
    blog_t<T>*     blog = ugraph->blog;
    index_t new_count = upper_power_of_two(total_edge_count);
    blog->blog_mask = new_count -1;

    //Batch and Make Graph
    double start = mywtime();
    for (index_t i = 0; i < total_edge_count; ++i) {
        ugraph->batch_edge(edges[i]);
    }
    double end = mywtime ();
    cout << "Batch Update Time = " << end - start << endl;
}

template <class T>
void plaingraph_manager_t<T>::prep_graph_edgelog_fromtext(const string& idirname, 
        const string& odirname, typename callback<T>::parse_fn2_t parsefile_fn)
{
    pgraph_t<T>* pgraph = (pgraph_t<T>*)get_plaingraph();
    
    //Batch and Make Graph
    double start = mywtime();
    read_idir_text2(idirname, odirname, pgraph, parsefile_fn);    
    double end = mywtime();
    cout << "Batch Update Time = " << end - start << endl;
}

template <class T>
void plaingraph_manager_t<T>::prep_graph_fromtext(const string& idirname, 
        const string& odirname, typename callback<T>::parse_fn_t parsefile_fn)
{
    pgraph_t<T>* ugraph = (pgraph_t<T>*)get_plaingraph();
    
    //Batch and Make Graph
    double start = mywtime();
    read_idir_text(idirname, odirname, ugraph, parsefile_fn);    
    double end = mywtime();
    cout << "Batch Update Time = " << end - start << endl;
    
    //----------
    g->waitfor_archive();
    end = mywtime();
    cout << "Make graph time = " << end - start << endl;
    //---------
}

template <class T>
void plaingraph_manager_t<T>::prep_graph_fromtext2(const string& idirname, 
        const string& odirname, typename callback<T>::parse_fn2_t parsebuf_fn)
{
    pgraph_t<T>* ugraph = (pgraph_t<T>*)get_plaingraph();
    
    //Batch and Make Graph
    double start = mywtime();
    read_idir_text2(idirname, odirname, ugraph, parsebuf_fn);    
    double end = mywtime();
    cout << "Batch Update Time = " << end - start << endl;
    
    //----------
    g->waitfor_archive();
    end = mywtime();
    cout << "Make graph time = " << end - start << endl;
    //---------
}

template <class T>
void plaingraph_manager_t<T>::prep_graph(const string& idirname, const string& odirname)
{
    edgeT_t<T>* edges = 0;
    index_t total_edge_count = read_idir(idirname, &edges, true); 
    
    pgraph_t<T>* ugraph = (pgraph_t<T>*)get_plaingraph();
    
    //Batch and Make Graph
    double start = mywtime();
    for (index_t i = 0; i < total_edge_count; ++i) {
        ugraph->batch_edge(edges[i]);
    }
    
    double end = mywtime ();
    cout << "Batch Update Time = " << end - start << endl;

    //----------
    g->waitfor_archive();
    end = mywtime();
    cout << "Make graph time = " << end - start << endl;
    //---------
}

template <class T>
void plaingraph_manager_t<T>::waitfor_archive_durable(double start) 
{
    pgraph_t<T>* pgraph = (pgraph_t<T>*)get_plaingraph();
    blog_t<T>* blog = pgraph->blog;
    index_t marker = blog->blog_head;
    if (marker != blog->blog_marker) {
        pgraph->create_marker(marker);
    }
    double end = 0;
    bool done_making = false;
    bool done_persisting = false;
    while (!done_making || !done_persisting) {
        if (blog->blog_tail == blog->blog_head && !done_making) {
            end = mywtime();
            cout << "Make Graph Time = " << end - start << endl;
            done_making = true;
        }
        if (blog->blog_wtail == blog->blog_head && !done_persisting) {
            end = mywtime();
            cout << "Durable Graph Time = " << end - start << endl;
            done_persisting = true;
        }
        usleep(1);
    }
}

template<class T>
void* sstream_func(void* arg) 
{
    sstream_t<T>* sstreamh = (sstream_t<T>*)arg;
    sstreamh->sstream_func(sstreamh);
    return 0;
}

template <class T>
void plaingraph_manager_t<T>::prep_graph_and_scompute(const string& idirname, const string& odirname, sstream_t<T>* sstreamh)
{
    g->create_threads(true, false);   
    
    edgeT_t<T>* edges = 0;
    index_t total_edge_count = read_idir(idirname, &edges, true); 
    
    pgraph_t<T>* ugraph = (pgraph_t<T>*)get_plaingraph();
    
    //Create a thread for stream pagerank
    pthread_t sstream_thread;
    if (0 != pthread_create(&sstream_thread, 0, &sstream_func<T>, sstreamh)) {
        assert(0);
    }
    
    //Batch and Make Graph
    double start = mywtime();
    for (index_t i = 0; i < total_edge_count; ++i) {
        ugraph->batch_edge(edges[i]);
    }
    //---------
    double end = mywtime ();
    cout << "Batch Update Time = " << end - start << endl;
    g->waitfor_archive();
    end = mywtime();
    cout << " Make graph time = " << end - start << endl;
    //---------
    
    void* ret;
    pthread_join(sstream_thread, &ret);
}

template <class T>
void plaingraph_manager_t<T>::prep_graph_serial_scompute(const string& idirname, const string& odirname, sstream_t<T>* sstreamh)
{
    edgeT_t<T>* edges = 0;
    index_t total_edge_count = read_idir(idirname, &edges, true); 
    
    pgraph_t<T>* ugraph = (pgraph_t<T>*)get_plaingraph();
    
    //Create a thread for make graph and stream pagerank
    pthread_t sstream_thread;
    if (0 != pthread_create(&sstream_thread, 0, &sstream_func<T>, sstreamh)) {
        assert(0);
    }
    cout << "created stream thread" << endl;
    
    //Batch and Make Graph
    double start = mywtime();
    for (index_t i = 0; i < total_edge_count; ++i) {
        ugraph->batch_edge(edges[i]);
    }
    double end = mywtime ();
    cout << "Batch Update Time = " << end - start << endl;
    
    void* ret;
    pthread_join(sstream_thread, &ret);
    //while (true) usleep(10);
}

template <class T>
void plaingraph_manager_t<T>::prep_graph_durable(const string& idirname, const string& odirname)
{
    edgeT_t<T>* edges = 0;
    index_t total_edge_count = read_idir(idirname, &edges, true); 
    
    pgraph_t<T>* ugraph = (pgraph_t<T>*)get_plaingraph();
    
    //Batch and Make Graph
    double start = mywtime();
    for (index_t i = 0; i < total_edge_count; ++i) {
        ugraph->batch_edge(edges[i]);
    }
    
    double end = mywtime ();
    cout << "Batch Update Time = " << end - start << endl;

    //Wait for make and durable graph
    waitfor_archive_durable(start);
}

#include "mem_iterative_analytics.h"

template <class T>
void plaingraph_manager_t<T>::run_pr() 
{
    pgraph_t<T>* pgraph = (pgraph_t<T>*)get_plaingraph();
    snap_t<T>* snaph = create_static_view(pgraph, true, true, true);
    
    mem_pagerank<T>(snaph, 5);
    delete_static_view(snaph);
}

template <class T>
void plaingraph_manager_t<T>::run_pr_simple()
{
    pgraph_t<T>* pgraph = (pgraph_t<T>*)get_plaingraph();
    double start = mywtime();
    snap_t<T>* snaph = create_static_view(pgraph, true, true, true);
    double end = mywtime();
    cout << "static View creation = " << end - start << endl;    
    
    mem_pagerank_simple<T>(snaph, 5);
    
    delete_static_view(snaph);
}
template <class T>
void plaingraph_manager_t<T>::run_bfs(sid_t root/*=1*/)
{
    double start, end;

    pgraph_t<T>* pgraph1 = (pgraph_t<T>*)get_plaingraph();
    
    start = mywtime();
    snap_t<T>* snaph = create_static_view(pgraph1, true, true, true);
    end = mywtime();
    cout << "static View creation = " << end - start << endl;    
    
    uint8_t* level_array = 0;
    level_array = (uint8_t*) calloc(snaph->get_vcount(), sizeof(uint8_t));
    start = mywtime();
    mem_bfs<T>(snaph, level_array, root);
    end = mywtime();
    cout << "BFS complex = " << end - start << endl;    
    
    free(level_array);
    level_array = (uint8_t*) calloc(snaph->get_vcount(), sizeof(uint8_t));
    start = mywtime();
    mem_bfs_simple<T>(snaph, level_array, root);
    end = mywtime();
    cout << "BFS simple = " << end - start << endl;    
    
    delete_static_view(snaph);
}

template <class T>
void plaingraph_manager_t<T>::run_1hop()
{
    pgraph_t<T>* pgraph1 = (pgraph_t<T>*)get_plaingraph();
    snap_t<T>* snaph = create_static_view(pgraph1, true, true, true);
    mem_hop1<T>(snaph);
    delete_static_view(snaph);
}

template <class T>
void plaingraph_manager_t<T>::run_2hop()
{
    pgraph_t<T>* pgraph1 = (pgraph_t<T>*)get_plaingraph();
    snap_t<T>* snaph = create_static_view(pgraph1, true, true, true);
    mem_hop2<T>(snaph);
    delete_static_view(snaph);
}

#include "stream_analytics.h"

template <class T>
void plaingraph_manager_t<T>::prep_graph_and_compute(const string& idirname, const string& odirname, stream_t<T>* streamh)
{
    pgraph_t<T>* ugraph = (pgraph_t<T>*)get_plaingraph();
    blog_t<T>*     blog = ugraph->blog;
    
    blog->blog_head  += read_idir(idirname, &blog->blog_beg, false);
    
    double start = mywtime();
    
    index_t marker = 0, prior_marker = 0;
    index_t batch_size = (1L << residue);
    cout << "batch_size = " << batch_size << endl;

    while (marker < blog->blog_head) {
        marker = min(blog->blog_head, marker+batch_size);
        
        //This is like a controlled update_stream_view() call
        streamh->set_edgecount(marker - prior_marker);
        streamh->set_edges(blog->blog_beg + prior_marker);
        streamh->edge_offset = prior_marker;
        streamh->stream_func(streamh);
        prior_marker = marker;
    }
    double end = mywtime ();
    cout << "Make graph time = " << end - start << endl;
}


extern plaingraph_manager_t<sid_t> plaingraph_manager; 
