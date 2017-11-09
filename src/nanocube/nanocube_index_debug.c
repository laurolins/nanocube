// depends on base/aatree to associate nodes with an monotonically
// incresing number

typedef struct {
	pt_Memory          print_buffer;
	Print              print;
	pt_File            file;
	u64                msg_threads_count;
	u64                msg_threads_dimension;

	char               *stack[28]; // message string to print on pop
	s32                stack_size;
} nx_NanocubeIndexLog;

global_variable b8                  nx_log_initialized = 0;
global_variable nx_NanocubeIndexLog nx_log;

internal void
nx_log_indent()
{
	Print_nchar(&nx_log.print, ' ', nx_log.stack_size * 4);
}

internal void
nx_log_dump()
{
	Assert(nx_log_initialized);
	platform.write_to_file(&nx_log.file, nx_log.print.begin, nx_log.print.end);
	Print_clear(&nx_log.print);
}

internal void
nx_log_write_node_path(nx_NanocubeIndex* h, int index, nx_Node* node, int prefix_size)
{
	nx_PathToRootInfo info;
	nx_PathToRootInfo_init(&info);
	while (node) {
		nx_PathToRootInfo_reset(&info, h, index, node, prefix_size);
		//         auto &path = info._path;
		for (int i=0;i<nx_List_u8_size(&info.path);++i) {
			if (i > 0) Print_char(&nx_log.print,',');
			Print_u64(&nx_log.print,(u64) nx_List_u8_get(&info.path,i));
		}
		--index;
		node = nx_Node_parent(nx_List_NodeP_get_reverse(&info.nodes,0));
		if (node) {
			prefix_size = node->path_length;
			Print_cstr(&nx_log.print,"/");
		}
	}
}

internal void
nx_log_msg_mfthreads_advance(int index, nx_Array arr)
{
	nx_log_indent();
	Print_format(&nx_log.print, "<event type=\"mfthreads_advance\" len=\"%d\" path=\"", arr.length);
	b8 first = 1;
	for (u8* it = arr.begin;it != nx_Array_end(&arr); ++it) {
		if (!first) Print_char(&nx_log.print, ',');
		Print_u64(&nx_log.print, (u64) *it);
		first = 0;
	}
	Print_cstr(&nx_log.print, "\"/>\n");
	nx_log_dump();
}

internal void
nx_log_msg_mfthreads_rewind(int index, int len)
{
	nx_log_indent();
	Print_format(&nx_log.print, "<event type=\"mfthreads_rewind\" len=\"%d\"/>\n", len);
	nx_log_dump();
}

internal void
nx_log_msg_mfthreads_enter(int index)
{
	++nx_log.msg_threads_dimension;
	nx_log_indent();
	Print_format(&nx_log.print, "<event type=\"mfthreads_enter\" dimension=\"%d\"/>\n", nx_log.msg_threads_dimension);
	nx_log_dump();

//
// 	++nx_log.msg_threads_dimension;
// 	Print_cstr(&nx_log.print, "[log_insert] " );
// 	Print_nchar(&nx_log.print, ' ', (u64) (index+1)*4);
// 	Print_cstr(&nx_log.print, "mfthreads enter dimension " );
// 	Print_u64(&nx_log.print, (u64) nx_log.msg_threads_dimension );
// 	Print_char(&nx_log.print, '\n');
// 	nx_log_dump();
}

internal void
nx_log_msg_mfthreads_exit(int index)
{
	nx_log_indent();
	Print_format(&nx_log.print, "<event type=\"mfthreads_exit\" dimension=\"%d\"/>\n", nx_log.msg_threads_dimension);
	nx_log_dump();

// 	Print_cstr(&nx_log.print, "[log_insert] " );
// 	Print_nchar(&nx_log.print, ' ', (index+1)*4);
// 	Print_cstr(&nx_log.print, "mfthreads exit dimension " );
// 	Print_u64(&nx_log.print, (u64) nx_log.msg_threads_dimension );
// 	Print_char(&nx_log.print, '\n');
//
	--nx_log.msg_threads_dimension;
}


internal void
nx_log_msg_mfthreads_push(int index)
{
	++nx_log.msg_threads_count;
	nx_log_indent();
	Print_format(&nx_log.print, "<event type=\"mfthreads_push\" num_minimally_finer_threads=\"%d\"/>\n", nx_log.msg_threads_count);
	nx_log_dump();
}

internal void
nx_log_msg_mfthreads_pop(int index)
{
	--nx_log.msg_threads_count;
	nx_log_indent();
	Print_format(&nx_log.print, "<event type=\"mfthreads_pop\" num_minimally_finer_threads=\"%d\"/>\n", nx_log.msg_threads_count);
	nx_log_dump();
}

internal void
nx_log_msg_clone_path(int index, int len)
{
	if (len == 0) return;
	nx_log_indent();
	Print_format(&nx_log.print, "<event type=\"clone_path\" length=\"%d\"/>\n", len);
	nx_log_dump();
}

internal void
nx_log_msg_print_str(int index, const char *msg)
{
	nx_log_indent();
	Print_format(&nx_log.print, "<event type=\"msg\" text=\"%s\"/>\n", (char*) msg);
	nx_log_dump();
}


internal void
nx_log_push_record(int i)
{
	nx_log_indent();
	Print_format(&nx_log.print, "<record number=\"%d\">\n", i);

	nx_log.stack[nx_log.stack_size] = "</record>\n";
	++nx_log.stack_size;

	nx_log_dump();
}

internal u64
nx_log_graph_get_node_id(nx_NanocubeIndex *nanocube, void *p)
{
	// null pointers all point to special key zero
	if (p == 0)
		return 0;
	u64 id = (u64)((char*) p - (char*) nanocube);
	return id;
}

internal void
nx_log_upstream_insert(int index, int i, int j, nx_NanocubeIndex *nanocube, nx_Node* node)
{
	u64 node_id = nx_log_graph_get_node_id(nanocube, node);

	nx_log_indent();
	Print_format(&nx_log.print, "<upstream_insert at=\"%d\" of=\"%d\" node=\"%llu\">\n", i, j, node_id);

	nx_log.stack[nx_log.stack_size] = "</upstream_insert>\n";
	++nx_log.stack_size;

	nx_log_dump();
}

internal void
nx_log_push_insert_case(nx_PositionType p, b8 shared, int index, nx_NanocubeIndex *nanocube_index, nx_Node *node)
{
	u64 node_id = nx_log_graph_get_node_id(nanocube_index, node);

	nx_log_indent();
	Print_format(&nx_log.print, "<insert case=\"%s\" shared=\"%d\" level=\"%d\" node=\"%llu\">\n", nx_pos_text[(int)p], shared, index, node_id);
	nx_log_dump();

	nx_log.stack[nx_log.stack_size] = "</insert>\n";
	++nx_log.stack_size;
}

internal void
nx_log_pop()
{
	Assert(nx_log.stack_size > 0);
	--nx_log.stack_size;
	nx_log_indent();
	Print_cstr(&nx_log.print, nx_log.stack[nx_log.stack_size]);
	nx_log_dump();
}

internal void
nx_log_msg_print_path(int index, nx_NanocubeIndex* h, const char *msg, nx_Node* node, int prefix_size)
{
	nx_log_indent();
	Print_format(&nx_log.print, "<path msg=\"%s\" path=\"", msg);
	nx_log_write_node_path(h, index, node, prefix_size);
	Print_cstr(&nx_log.print, "\"/>\n");
	nx_log_dump();
// 	nx_log.stack[nx_log.stack_size] = "</insert>\n";
// 	++nx_log.stack_size;
// 	Print_cstr(&nx_log.print, "[log_insert] " );
// 	Print_nchar(&nx_log.print, ' ', (index+1)*4);
// 	Print_cstr(&nx_log.print, msg );
// 	Print_char(&nx_log.print, ' ');
// 	nx_log_write_node_path(h, index, node, prefix_size);
// 	Print_char(&nx_log.print, '\n');
// 	nx_log_dump();
}

internal void
nx_log_append_path(nx_LabelArrayList *list)
{
	nx_log_indent();
	Print_cstr(&nx_log.print, "<append path=\"" );
	b8 sep = 0;
	while (list) {
		if (sep) {
			Print_cstr(&nx_log.print, "/");
		}
		b8 first = 1;
		u8 *it   = list->labels.begin;
		u8 *end  = it + list->labels.length;
		while (it != end) {
			if (!first)
				Print_char(&nx_log.print, ',');
			Print_u64(&nx_log.print, (u64) *it);
			first = 0;
			++it;
		}
		sep = 1;
		list = list->next;
	}
	Print_cstr(&nx_log.print, "\"/>\n");
	nx_log_dump();
}

internal void
nx_log_msg_print_mfheads(int index, nx_NanocubeIndex* h, nx_Threads *mfthreads)
{
	nx_log_indent();
	Print_cstr(&nx_log.print, "<mfheads>\n");

	// push
	nx_log.stack[nx_log.stack_size] = "</mfheads>\n";
	++nx_log.stack_size;

// 	u8          buf[nx_MAX_PATH_LENGTH];
// 	nx_List_u8  path;
// 	nx_List_u8_init(&path, buf, buf, buf + sizeof(buf));

	nx_DepthList depths;

	for (u32 i=0;i<mfthreads->size;++i) {
		nx_Thread *thread = mfthreads->threads + i;

		// compute thread path to root
		nx_DepthList_reset(&depths,
				   nx_Thread_head(thread),
				   nx_Thread_head_offset(thread) + nx_Thread_head_prefix_size(thread));

		nx_Node *head   = nx_Thread_head(thread);
		u64     node_id = nx_log_graph_get_node_id(h, head);

		nx_log_indent();
		Print_format(&nx_log.print, "<head index=\"%d\" node=\"%llu\" prefix_size=\"%d\" depths=\"", index,
			     node_id, nx_Thread_head_prefix_size(thread));
		for (s32 i=0;i<depths.length;++i) {
			if (i > 0)
				Print_char(&nx_log.print, ' ');
			Print_s64(&nx_log.print, (s64) depths.depth[i]);
		}
//
// 		u8 *it = path.begin;
// 		while (it != path.end) {
// 			if (it != path.begin)
// 				Print_char(&nx_log.print, ' ');
// 			Print_u64(&nx_log.print, (u64) *it);
// 			++it;
// 		}
//

		Print_cstr(&nx_log.print, "\"/>\n");
		nx_log_dump();
	}

	nx_log_pop();
}


internal void
nx_log_graph_node_event(nx_NanocubeIndex *nanocube, nx_Node* node, s32 index, char *event)
{
	u64 id = nx_log_graph_get_node_id(nanocube, node);
	nx_NodeWrap node_w = nx_NanocubeIndex_to_node(nanocube, node, index);
	nx_log_indent();
	Print_format(&nx_log.print, "<node event=\"%s\" id=\"%llu\" path=\"", event, id);
	for (s32 i=0;i<node_w.path.length;++i) {
		u8 label = nx_Path_get(&node_w.path, i);
		if (i > 0) {
			Print_char(&nx_log.print, ' ');
		}
		Print_u64(&nx_log.print, label);
	}
	Print_cstr(&nx_log.print, "\" children=\"");
	for (s32 i=0;i<node_w.children.length;++i) {
		nx_Child *child = node_w.children.begin + i;
		nx_Node  *child_node = nx_Child_get_node(child);
		u64 child_id = nx_log_graph_get_node_id(nanocube, child_node);
		if (i > 0) {
			Print_char(&nx_log.print, ' ');
		}
		Print_format(&nx_log.print, "%llu%s%d", child_id, (child->shared ? "s" : "p"), child->suffix_length);
	}
	u64 content_id;
	if (index < nanocube->dimensions - 1) {
		content_id = nx_log_graph_get_node_id(nanocube, nx_INode_content((nx_INode*) node));
	} else {
		// on PNodes state its content as its own id
		content_id = id;
	}
	Print_format(&nx_log.print, "\" content=\"%d\" isroot=\"%d\" dim=\"%d\"/>\n", content_id, node->root, index);
}

internal void
nx_log_start()
{
	Assert(!nx_log_initialized);
	nx_log = (nx_NanocubeIndexLog) {
		.msg_threads_count = 0,
		.msg_threads_dimension = 0,
		.stack_size = 0
	};

	static char *log_filename = "nanocube_index_log.xml";
	nx_log.file = platform.open_write_file(log_filename, cstr_end(log_filename));
	Assert(nx_log.file.open);

	// print
	u64 print_buffer_size = Megabytes(1);
	nx_log.print_buffer = platform.allocate_memory(print_buffer_size,0,0);
	Print_init(&nx_log.print, nx_log.print_buffer.memblock.begin,  nx_log.print_buffer.memblock.end);

	// log file initialized
	nx_log_initialized = 1;

	// push a first tag with the log
	nx_log_indent();
	Print_format(&nx_log.print, "<log>\n");
	nx_log.stack[nx_log.stack_size] = "</log>\n";
	++nx_log.stack_size;
	nx_log_dump();

}

internal void
nx_log_finish()
{
	Assert(nx_log_initialized);
	nx_log_pop();
	Assert(nx_log.stack_size == 0);
	platform.close_file(&nx_log.file);
	platform.free_memory(&nx_log.print_buffer);
	nx_log_initialized = 0;
}

#define nx_LOG_START(a) (nx_log_start())
#define nx_LOG_FINISH(a) (nx_log_finish())
#define nx_LOG_PUSH_INSERT_CASE(a,b,c,d,e) (nx_log_push_insert_case(a,b,c,d,e))
#define nx_LOG_PUSH_RECORD(a) (nx_log_push_record(a))
#define nx_LOG_APPEND_PATH(a) (nx_log_append_path(a))
#define nx_LOG_NODE_EVENT(a,b,c,d) (nx_log_graph_node_event(a,b,c,d))
#define nx_LOG_POP(a) (nx_log_pop())
#define nx_LOG_MFTHREADS_ADVANCE(a,b) (nx_log_msg_mfthreads_advance(a,b))
#define nx_LOG_MFTHREADS_REWIND(a,b) (nx_log_msg_mfthreads_rewind(a,b))
#define nx_LOG_MFTHREADS_ENTER(a) (nx_log_msg_mfthreads_enter(a))
#define nx_LOG_MFTHREADS_EXIT(a) (nx_log_msg_mfthreads_exit(a))
#define nx_LOG_MFTHREADS_PUSH(a) (nx_log_msg_mfthreads_push(a))
#define nx_LOG_MFTHREADS_POP(a) (nx_log_msg_mfthreads_pop(a))
#define nx_LOG_MFTHREADS_CLONE_PATH(a,b) (nx_log_msg_clone_path(a,b))
#define nx_LOG_UPSTREAM_INSERT(a,b,c,d,e) (nx_log_upstream_insert(a,b,c,d,e))
#define nx_LOG_PRINT_MFHEADS(a,b,c) (nx_log_msg_print_mfheads(a,b,c))
#define nx_LOG_PRINT_STR(a,b) (nx_log_msg_print_str(a,b))
#define nx_LOG_PRINT_PATH(a,b,c,d,e) (nx_log_msg_print_path(a,b,c,d,e))
#define nx_LOG_RESET_COUNTERS (nx_log_reset_counters())
