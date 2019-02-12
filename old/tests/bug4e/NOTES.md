nx_Nanocube_upstream_insert(nx_NanocubeIndex  *self,
                            nx_LabelArrayList *insert_path_list,
                            int               index,
                            nx_Threads        *mfthreads,
                            void              *payload_unit,
                            void              *payload_context,
                            nx_Node           *mfthread, // minimally finer thread is a root in the next dimension or a PNode
                            int               num_shared,
                            nx_List_u8        *lengths,
                            nx_List_NodeP     *path)
--
nx_INSERT_CASE(nx_NanocubeIndex_insert_exact)
        nx_Nanocube_upstream_insert(self,
                                    insert_path_list,
                                    index,
                                    mfthreads,
                                    payload_unit,
                                    payload_context,
                                    0,                     // <----- no new minimally finer thread 
                                    shared ? (s32) nx_List_NodeP_size(&pos->path) : 0,
                                    &pos->lengths,
                                    &pos->path);
--
nx_INSERT_CASE(nx_NanocubeIndex_insert_branch)
        nx_Nanocube_upstream_insert(self,
                                    insert_path_list,
                                    index,
                                    mfthreads,
                                    payload_unit,
                                    payload_context,
                                    (index < self->dimensions - 1) ? nx_INode_content((nx_INode*)new_node) : 0, // probably a bug: branch means<----- no new minimally finer thread 
                                    shared ? (s32) nx_List_NodeP_size(&pos->path) : 0,
                                    &pos->lengths,
                                    &pos->path);
--
nx_INSERT_CASE(nx_NanocubeIndex_insert_split)
        nx_Nanocube_upstream_insert(self,
                                    insert_path_list,
                                    index,
                                    mfthreads,
                                    payload_unit,
                                    payload_context,
                                    (index < self->dimensions - 1) ? nx_INode_content((nx_INode*)new_node) : 0,
                                    shared ? (s32) nx_List_NodeP_size(&pos->path) : 1,
                                    &pos->lengths,
                                    &pos->path);
--
nx_INSERT_CASE(nx_NanocubeIndex_insert_shared_suffix_was_split)
        nx_Nanocube_upstream_insert(self,
                                    insert_path_list,
                                    index,
                                    mfthreads,
                                    payload_unit,
                                    payload_context,
                                    (index < self->dimensions - 1) ? nx_INode_content((nx_INode*)child) : 0,
                                    shared ? (s32)nx_List_NodeP_size(&pos->path) : 0,
                                    &pos->lengths,
                                    &pos->path);
--
nx_INSERT_CASE(nx_NanocubeIndex_insert_shared_split_or_shared_no_split)
        nx_Nanocube_upstream_insert(self,
                                    insert_path_list,
                                    index,
                                    mfthreads,
                                    payload_unit,
                                    payload_context,
                                    0,
                                    new_proper_nodes,
                                    &pos->lengths,
                                    &pos->path);
