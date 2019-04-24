#pragma once

#include "type.h" 

extern vid_t RANGE_COUNT;
/*
template <class T>
void pgraph_t<T>::alloc_edge_buf(index_t total) 
{
    index_t total_edge_count = 0;
    if (0 == sgraph_in) {
        total_edge_count = (total << 1);
        if (0 == edge_buf_count) {
            edge_buf_out = (edgeT_t<T>*)malloc(total_edge_count*sizeof(edgeT_t<T>));
            edge_buf_count = total_edge_count;
        } else if (edge_buf_count < total_edge_count) {
            free(edge_buf_out);
            edge_buf_out = (edgeT_t<T>*)malloc(total_edge_count*sizeof(edgeT_t<T>));
            edge_buf_count = total_edge_count;
        }
    } else {
        total_edge_count = total;
        if (0 == edge_buf_count) {
            edge_buf_out = (edgeT_t<T>*)malloc(total_edge_count*sizeof(edgeT_t<T>));
            edge_buf_in = (edgeT_t<T>*)malloc(total_edge_count*sizeof(edgeT_t<T>));
            edge_buf_count = total_edge_count;
        } else if (edge_buf_count < total_edge_count) {
            free(edge_buf_out);
            free(edge_buf_in);
            edge_buf_out = (edgeT_t<T>*)malloc(total_edge_count*sizeof(edgeT_t<T>));
            edge_buf_in = (edgeT_t<T>*)malloc(total_edge_count*sizeof(edgeT_t<T>));
            edge_buf_count = total_edge_count;
        }
    }
}

template <class T>
void pgraph_t<T>::free_edge_buf() 
{
    if (edge_buf_out) {    
        free(edge_buf_out);
        edge_buf_out = 0;
    }
    if (edge_buf_in) {
        free(edge_buf_in);
        edge_buf_in = 0;
    }
    edge_buf_count = 0;
}
*/

template <class T>
class edge_shard_t {
 public:
    blog_t<T>* blog;
    
    //intermediate classification buffer
    edgeT_t<T>* edge_buf_out;
    edgeT_t<T>* edge_buf_in;
    index_t edge_buf_count;
    
    global_range_t<T>* global_range;
    global_range_t<T>* global_range_in;
    
    thd_local_t* thd_local;
    thd_local_t* thd_local_in;

 public:
    ~edge_shard_t();
    edge_shard_t(blog_t<T>* binary_log);
    void classify_u();
    void classify_d();
    void classify_uni();
    void classify_runi();
    void cleanup();

 private:
    void estimate_classify(vid_t* vid_range, vid_t* vid_range_in, vid_t bit_shift, vid_t bit_shift_in);
    void estimate_classify_uni(vid_t* vid_range, vid_t bit_shift);
    void estimate_classify_runi(vid_t* vid_range, vid_t bit_shift);

    void prefix_sum(global_range_t<T>* global_range, thd_local_t* thd_local,
                             vid_t range_count, vid_t thd_count, edgeT_t<T>* edge_buf);

    void work_division(global_range_t<T>* global_range, thd_local_t* thd_local,
                        vid_t range_count, vid_t thd_count, index_t equal_work);

    void classify(vid_t* vid_range, vid_t* vid_range_in, vid_t bit_shift, vid_t bit_shift_in, 
            global_range_t<T>* global_range, global_range_t<T>* global_range_in);
    void classify_runi(vid_t* vid_range, vid_t bit_shift, global_range_t<T>* global_range);
    void classify_uni(vid_t* vid_range, vid_t bit_shift, global_range_t<T>* global_range);

};

template <class T>
edge_shard_t<T>::edge_shard_t(blog_t<T>* binary_log)
{
    blog = binary_log;
    //alloc_edge_buf(total_edge_count);
    global_range = (global_range_t<T>*)calloc(RANGE_COUNT, 
                                        sizeof(global_range_t<T>));
    global_range_in = (global_range_t<T>*)calloc(RANGE_COUNT, 
                                        sizeof(global_range_t<T>));
    
    thd_local = (thd_local_t*) calloc(THD_COUNT, sizeof(thd_local_t));  
    thd_local_in = (thd_local_t*) calloc(THD_COUNT, sizeof(thd_local_t));  
}

template <class T>
edge_shard_t<T>::~edge_shard_t()
{
    //free_edge_buf();
    free(global_range);
    free(thd_local);
    free(global_range_in);
    free(thd_local_in);
}

template <class T>
void edge_shard_t<T>::cleanup()
{
    #pragma omp for schedule (static)
    for (vid_t i = 0; i < RANGE_COUNT; ++i) {
        if (global_range[i].edges) {
            free(global_range[i].edges);
            global_range[i].edges = 0;
            global_range[i].count = 0;
        }
        if (global_range_in[i].edges) {
            free(global_range_in[i].edges);
            global_range_in[i].edges = 0;
            global_range_in[i].count = 0;
        }
    }
    #pragma omp for schedule (static)
    for (int i = 0; i < THD_COUNT; ++i) {
        thd_local[i].range_end = 0;
        thd_local_in[i].range_end = 0;
    }
}

template <class T>
void edge_shard_t<T>::classify_u()
{
    index_t total_edge_count = blog->blog_marker - blog->blog_tail ;
    index_t edge_count = ((total_edge_count << 1)*1.15)/(THD_COUNT);
    
    tid_t src_index = TO_TID(blog->blog_beg[0].src_id);
    vid_t v_count = g->get_type_vcount(src_index);
    
    vid_t  base_vid = ((v_count -1)/RANGE_COUNT);
    
    //find the number of bits to do shift to find the range
#if B32    
    vid_t bit_shift = __builtin_clz(base_vid);
    bit_shift = 32 - bit_shift; 
#else
    vid_t bit_shift = __builtin_clzl(base_vid);
    bit_shift = 64 - bit_shift; 
#endif
    
    //#pragma omp parallel num_threads(thd_count)
    int tid = omp_get_thread_num();
    vid_t* vid_range = (vid_t*)calloc(RANGE_COUNT, sizeof(vid_t)); 
    thd_local[tid].vid_range = vid_range;

    //Get the count for classification
    this->estimate_classify(vid_range, vid_range, bit_shift, bit_shift);
    
    this->prefix_sum(global_range, thd_local, RANGE_COUNT, THD_COUNT, edge_buf_out);
    #pragma omp barrier 
    
    //Classify
    this->classify(vid_range, vid_range, bit_shift, bit_shift, global_range, global_range);
    #pragma omp master 
    {
        //print(" classify = ", start);
        this->work_division(global_range, thd_local, RANGE_COUNT, THD_COUNT, edge_count);
    }
    #pragma omp barrier 
    free(vid_range);
}

template <class T>
void edge_shard_t<T>::classify_uni()
{
    index_t total_edge_count = blog->blog_marker - blog->blog_tail ;
    index_t edge_count = ((total_edge_count << 1)*1.15)/(THD_COUNT);
    
    tid_t src_index = TO_TID(blog->blog_beg[0].src_id);
    vid_t v_count = g->get_type_vcount(src_index);
    
    vid_t  base_vid = ((v_count -1)/RANGE_COUNT);
    
    //find the number of bits to do shift to find the range
#if B32    
    vid_t bit_shift = __builtin_clz(base_vid);
    bit_shift = 32 - bit_shift; 
#else
    vid_t bit_shift = __builtin_clzl(base_vid);
    bit_shift = 64 - bit_shift; 
#endif
    
    int tid = omp_get_thread_num();
    vid_t* vid_range = (vid_t*)calloc(RANGE_COUNT, sizeof(vid_t)); 
    thd_local[tid].vid_range = vid_range;

    //Get the count for classification
    this->estimate_classify_uni(vid_range, bit_shift);
    
    this->prefix_sum(global_range, thd_local, RANGE_COUNT, THD_COUNT, edge_buf_out);
    #pragma omp barrier 
    
    //Classify
    this->classify_uni(vid_range, bit_shift, global_range);
    #pragma omp master 
    {
        //print(" classify = ", start);
        this->work_division(global_range, thd_local, RANGE_COUNT, THD_COUNT, edge_count);
    }
    #pragma omp barrier 
    free(vid_range);
}

template <class T>
void edge_shard_t<T>::classify_runi()
{
    index_t total_edge_count = blog->blog_marker - blog->blog_tail ;
    index_t edge_count = ((total_edge_count << 1)*1.15)/(THD_COUNT);
    
    tid_t src_index = TO_TID(blog->blog_beg[0].src_id);
    vid_t v_count = g->get_type_vcount(src_index);
    
    vid_t  base_vid = ((v_count -1)/RANGE_COUNT);
    
    //find the number of bits to do shift to find the range
#if B32    
    vid_t bit_shift = __builtin_clz(base_vid);
    bit_shift = 32 - bit_shift; 
#else
    vid_t bit_shift = __builtin_clzl(base_vid);
    bit_shift = 64 - bit_shift; 
#endif
    
    int tid = omp_get_thread_num();
    vid_t* vid_range = (vid_t*)calloc(RANGE_COUNT, sizeof(vid_t)); 
    thd_local[tid].vid_range = vid_range;

    //Get the count for classification
    this->estimate_classify_runi(vid_range, bit_shift);
    
    this->prefix_sum(global_range, thd_local, RANGE_COUNT, THD_COUNT, edge_buf_out);
    #pragma omp barrier 
    
    //Classify
    this->classify_runi(vid_range, bit_shift, global_range);
    #pragma omp master 
    {
        //print(" classify = ", start);
        this->work_division(global_range, thd_local, RANGE_COUNT, THD_COUNT, edge_count);
    }
    #pragma omp barrier 
    free(vid_range);
}

template <class T>
void edge_shard_t<T>::classify_d()
{
    tid_t src_index = TO_TID(blog->blog_beg[0].src_id);
    vid_t v_count = g->get_type_vcount(src_index);
    vid_t  base_vid = ((v_count -1)/RANGE_COUNT);
    if (base_vid == 0) {
        base_vid = RANGE_COUNT;
    }
    
    tid_t dst_index = TO_TID(get_dst(&blog->blog_beg[0]));
    vid_t v_count_in = g->get_type_vcount(dst_index);
    vid_t  base_vid_in = ((v_count_in -1)/RANGE_COUNT);
    if (base_vid_in == 0) {
        base_vid_in = RANGE_COUNT;
    }
    
    //find the number of bits to do shift to find the range
#if B32    
    vid_t bit_shift = __builtin_clz(base_vid);
    bit_shift = 32 - bit_shift; 
    vid_t bit_shift_in = __builtin_clz(base_vid_in);
    bit_shift_in = 32 - bit_shift_in; 
#else
    vid_t bit_shift = __builtin_clzl(base_vid);
    bit_shift = 64 - bit_shift; 
    vid_t bit_shift_in = __builtin_clzl(base_vid_in);
    bit_shift_in = 64 - bit_shift_in; 
#endif

    index_t total_edge_count = blog->blog_marker - blog->blog_tail;
    //alloc_edge_buf(total_edge_count);
    
    index_t edge_count = (total_edge_count*1.15)/(THD_COUNT);

    vid_t tid = omp_get_thread_num();
    vid_t* vid_range = (vid_t*)calloc(sizeof(vid_t), RANGE_COUNT); 
    vid_t* vid_range_in = (vid_t*)calloc(sizeof(vid_t), RANGE_COUNT); 
    thd_local[tid].vid_range = vid_range;
    thd_local_in[tid].vid_range = vid_range_in;

    //double start = mywtime();

    //Get the count for classification
    this->estimate_classify(vid_range, vid_range_in, bit_shift, bit_shift_in);
    
    this->prefix_sum(global_range, thd_local, RANGE_COUNT, THD_COUNT, edge_buf_out);
    this->prefix_sum(global_range_in, thd_local_in, RANGE_COUNT, THD_COUNT, edge_buf_in);
    #pragma omp barrier 
    
    //Classify
    this->classify(vid_range, vid_range_in, bit_shift, bit_shift_in, global_range, global_range_in);
    
    #pragma omp master 
    {
        //double end = mywtime();
        //cout << " classify " << end - start << endl;
        this->work_division(global_range, thd_local, RANGE_COUNT, THD_COUNT, edge_count);
    }
    
    if (tid == 1) {
        this->work_division(global_range_in, thd_local_in, RANGE_COUNT, THD_COUNT, edge_count);
    }
    #pragma omp barrier 
    free(vid_range);
    free(vid_range_in);

}

template <class T>
void edge_shard_t<T>::estimate_classify(vid_t* vid_range, vid_t* vid_range_in, vid_t bit_shift, vid_t bit_shift_in) 
{
    sid_t src, dst;
    vid_t vert1_id, vert2_id;
    vid_t range;
    edgeT_t<T>* edges = blog->blog_beg;
    index_t index;

    #pragma omp for schedule(static)
    for (index_t i = blog->blog_tail; i < blog->blog_marker; ++i) {
        index = (i & blog->blog_mask);
        src = edges[index].src_id;
        dst = get_sid(edges[index].dst_id);
        
        vert1_id = TO_VID(src);
        vert2_id = TO_VID(dst);

        //gather high level info for 1
        range = (vert1_id >> bit_shift);
        assert(range < RANGE_COUNT);
        vid_range[range] += 1;
        
        //gather high level info for 2
        range = (vert2_id >> bit_shift_in);
        assert(range < RANGE_COUNT);
        vid_range_in[range] += 1;
    }
}

template <class T>
void edge_shard_t<T>::estimate_classify_uni(vid_t* vid_range, vid_t bit_shift) 
{
    sid_t src;
    vid_t vert1_id;
    vid_t range;
    edgeT_t<T>* edges = blog->blog_beg;
    index_t index;

    #pragma omp for
    for (index_t i = blog->blog_tail; i < blog->blog_marker; ++i) {
        index = (i & blog->blog_mask);
        src = edges[index].src_id;
        vert1_id = TO_VID(src);

        //gather high level info for 1
        range = (vert1_id >> bit_shift);
        vid_range[range] += 1;
    }
}

template <class T>
void edge_shard_t<T>::estimate_classify_runi(vid_t* vid_range, vid_t bit_shift) 
{
    sid_t dst;
    vid_t vert_id;
    vid_t range;
    edgeT_t<T>* edges = blog->blog_beg;
    index_t index;

    #pragma omp for
    for (index_t i = blog->blog_tail; i < blog->blog_marker; ++i) {
        index = (i & blog->blog_mask);
        dst = get_sid(edges[index].dst_id);
        vert_id = TO_VID(dst);

        //gather high level info for 1
        range = (vert_id >> bit_shift);
        vid_range[range]++;
    }
}

template <class T>
void edge_shard_t<T>::prefix_sum(global_range_t<T>* global_range, thd_local_t* thd_local,
                             vid_t range_count, vid_t thd_count, edgeT_t<T>* edge_buf)
{
    index_t total = 0;
    index_t value = 0;
    //index_t alloc_start = 0;

    //#pragma omp master
    #pragma omp for schedule(static) nowait
    for (vid_t i = 0; i < range_count; ++i) {
        total = 0;
        //global_range[i].edges = edge_buf + alloc_start;//good for larger archiving batch
        for (vid_t j = 0; j < thd_count; ++j) {
            value = thd_local[j].vid_range[i];
            thd_local[j].vid_range[i] = total;
            total += value;
        }

        //alloc_start += total;
        global_range[i].count = total;
        global_range[i].edges = 0;
        if (total != 0) {
            global_range[i].edges = (edgeT_t<T>*)calloc(total, sizeof(edgeT_t<T>));
            if (0 == global_range[i].edges) {
                cout << total << " bytes of allocation failed" << endl;
                assert(0);
            }
        }
    }
}

template <class T>
void edge_shard_t<T>::work_division(global_range_t<T>* global_range, thd_local_t* thd_local,
                        vid_t range_count, vid_t thd_count, index_t equal_work)
{
    index_t my_portion = global_range[0].count;
    index_t value;
    vid_t j = 0;
    
    for (vid_t i = 1; i < range_count && j < thd_count; ++i) {
        value = global_range[i].count;
        if (my_portion + value > equal_work && my_portion != 0) {
            //cout << j << " " << my_portion << endl;
            thd_local[j++].range_end = i;
            my_portion = 0;
        }
        my_portion += value;
    }

    if (j == thd_count)
        thd_local[j -1].range_end = range_count;
    else 
        thd_local[j++].range_end = range_count;
    
    /*
    my_portion = 0;
    vid_t i1 = thd_local[j - 2].range_end;
    for (vid_t i = i1; i < range_count; i++) {
        my_portion += global_range[i1].count;
    }
    cout << j - 1 << " " << my_portion << endl;
    */
}


template <class T>
void edge_shard_t<T>::classify(vid_t* vid_range, vid_t* vid_range_in, vid_t bit_shift, vid_t bit_shift_in, 
            global_range_t<T>* global_range, global_range_t<T>* global_range_in)
{
    sid_t src, dst;
    vid_t vert1_id, vert2_id;
    vid_t range = 0;
    edgeT_t<T>* edge;
    edgeT_t<T>* edges = blog->blog_beg;
    index_t index;

    #pragma omp for schedule(static)
    for (index_t i = blog->blog_tail; i < blog->blog_marker; ++i) {
        index = (i & blog->blog_mask);
        src = edges[index].src_id;
        dst = get_sid(edges[index].dst_id);
       
        vert1_id = TO_VID(src);
        vert2_id = TO_VID(dst);
        
        range = (vert1_id >> bit_shift);
        edge = global_range[range].edges + vid_range[range];
        vid_range[range] += 1;
        //assert(edge - global_range[range].edges < global_range[range].count);
        edge->src_id = src;
        edge->dst_id = edges[index].dst_id;
        
        range = (vert2_id >> bit_shift_in);
        edge = global_range_in[range].edges + vid_range_in[range];
        vid_range_in[range] += 1;
        //assert(edge - global_range_in[range].edges < global_range_in[range].count);
        edge->src_id = dst;
        set_dst(edge, src);
        set_weight(edge, edges[index].dst_id);
    }
}

template <class T>
void edge_shard_t<T>::classify_runi(vid_t* vid_range, vid_t bit_shift, global_range_t<T>* global_range)
{
    sid_t src, dst;
    vid_t vert_id;
    vid_t range = 0;
    edgeT_t<T>* edge;
    edgeT_t<T>* edges = blog->blog_beg;
    index_t index;

    #pragma omp for
    for (index_t i = blog->blog_tail; i < blog->blog_marker; ++i) {
        index = (i & blog->blog_mask);
        src = edges[index].src_id;
        dst = get_sid(edges[index].dst_id);
        vert_id = TO_VID(dst);
        
        range = (vert_id >> bit_shift);
        edge = global_range[range].edges + vid_range[range]++;
        
        edge->src_id = dst;
        set_dst(edge, src);
        set_weight(edge, edges[index].dst_id);
    }
}

template <class T>
void edge_shard_t<T>::classify_uni(vid_t* vid_range, vid_t bit_shift, global_range_t<T>* global_range)
{
    sid_t src;
    vid_t vert1_id;
    vid_t range = 0;
    edgeT_t<T>* edge;
    edgeT_t<T>* edges = blog->blog_beg;
    index_t index;

    #pragma omp for
    for (index_t i = blog->blog_tail; i < blog->blog_marker; ++i) {
        index = (i & blog->blog_mask);
        src = edges[index].src_id;
        vert1_id = TO_VID(src);
        
        range = (vert1_id >> bit_shift);
        edge = global_range[range].edges + vid_range[range]++;
        edge->src_id = src;
        edge->dst_id = edges[index].dst_id;
    }
}
