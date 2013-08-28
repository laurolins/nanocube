var TileInactive = 0;
var TileInQueue = 1;
var TileMidRequest = 2;
var TileReadyToDraw = 3;

function QuadTreeNode(zoom, x, y) {
                     // sw, se, nw, ne
    this.children = [undefined, undefined, undefined, undefined];
    this.zoom = zoom;
    this.x = x;
    this.y = y;

    this.tile_state = TileInactive;
    this.last_touched = 0;
};

QuadTreeNode.prototype.find_node = function(zoom, x, y)
{
    if (this.x === x && this.y === y && this.zoom === zoom)
        return this;

    // determine quadrant of children
    var next_x = this.x << 1;
    var next_y = this.y << 1;
    var child_x_along_next_zoom = x >> (zoom - this.zoom - 1);
    var child_y_along_next_zoom = y >> (zoom - this.zoom - 1);
    var dx = (next_x !== child_x_along_next_zoom);
    var dy = (next_y !== child_y_along_next_zoom);
    var ix = dx + (dy * 2);
    
    if (!this.children[ix])
        return undefined;
    else
        return this.children[ix].insert_child(zoom, x, y);
};

QuadTreeNode.prototype.insert_child = function(zoom, x, y)
{
    if (this.x === x && this.y === y && this.zoom === zoom)
        return this;
    // not found; create children if necessary, and recurse
    
    // determine quadrant of children
    var next_x = this.x << 1;
    var next_y = this.y << 1;
    var child_x_along_next_zoom = x >> (zoom - this.zoom - 1);
    var child_y_along_next_zoom = y >> (zoom - this.zoom - 1);
    var dx = (next_x !== child_x_along_next_zoom);
    var dy = (next_y !== child_y_along_next_zoom);
    var ix = dx + (dy * 2);

    if (!this.children[ix])
        this.children[ix] = new QuadTreeNode(this.zoom + 1, next_x + dx, next_y + dy);
    return this.children[ix].insert_child(zoom, x, y);
};

QuadTreeNode.prototype.level_visit = function(visit_node)
{
    if (visit_node(this)) {
        for (var i=0; i<4; ++i) {
            if (this.children[i])
                this.children[i].level_visit(visit_node);
        }
    }
};

function tile_cache(opts) {
    opts = _.defaults(opts, {
        max_size: 1024,
        compare_tile: function(cl1, cl2) {
            return cl1.zoom < cl2.zoom ? -1 :
                   cl1.zoom > cl2.zoom ?  1 :
                   cl1.x    < cl2.x    ? -1 :
                   cl1.x    > cl2.x    ?  1 :
                   cl1.y    < cl2.y    ? -1 :
                   cl1.y    > cl2.y    ?  1 :
                                          0;
        }
    });
    var max_size = opts.max_size;
    var root = new QuadTreeNode(0,0,0);

    var compare_tile = opts.compare_tile;

    var request_list = [];

    var max_inflight_requests = 6;

    var cache;

    function issue_new_requests() {
        if (request_list.length === 0 ||
            cache.midrequest_tiles_count >= max_inflight_requests)
            return;
        request_list.sort(compare_tile);
        for (var i=0; i<request_list.length && cache.midrequest_tiles_count<max_inflight_requests; ++i) {
            (function(the_request) {
                if (the_request.new_node.tile_state !== TileInQueue) {
                    debugger;
                }
                cache.midrequest_tiles_count++;
                the_request.new_node.tile_state = TileMidRequest;
                request_list.splice(i, 1);
                the_request.perform_request(the_request.new_node, function(data) {
                    the_request.new_node.tile_state = TileReadyToDraw;
                    the_request.new_node.last_touched = new Date().getTime();
                    cache.ready_tiles_count++;
                    cache.midrequest_tiles_count--;
                    the_request.on_success(the_request.new_node, data);
                    issue_new_requests();
                });
            })(request_list[i]);
        }
    }

    cache = {
        ready_tiles_count: 0,
        midrequest_tiles_count: 0,
        root: root,
        find_lru: function() {
            var current_best = this.root;
            function lru_visit_node(node) {
                if (node.last_touched < current_best.last_touched) {
                    current_best = node;
                }
                return true;
            }
            this.root.level_visit(lru_visit_node);
            return current_best;
        },
        visit_all_visible: function(screen_min_x, screen_max_x, screen_min_y, screen_max_y, visit) {
            var now = new Date().getTime();
            function internal_visit(node) {
                var span = 1 << node.zoom;
                var delta = 1 / span;
                var node_min_x = node.x / span,      node_max_y = 1 - node.y / span;
                var node_max_x = node_min_x + delta, node_min_y = node_max_y - delta;
                if (node_max_x < screen_min_x || node_min_x > screen_max_x ||
                    node_max_y < screen_min_y || node_min_y > screen_max_y) {
                    return false;
                } else {
                    if (node.tile_state === TileReadyToDraw) {
                        node.last_touched = now;
                        return visit(node);
                    }
                    return true;
                }
            }
            this.root.level_visit(internal_visit);
        },
        insert: function(zoom, x, y, perform_request, on_success) {
            var that = this;
            var new_node = this.root.insert_child(zoom, x, y);
            if (new_node.tile_state === TileInactive) {
                new_node.tile_state = TileInQueue;
                request_list.push({
                    zoom: zoom,
                    x: x,
                    y: y,
                    perform_request: perform_request,
                    on_success: on_success,
                    new_node: new_node
                });
                issue_new_requests();
                // this.midrequest_tiles_count++;
                // perform_request(new_node, function(data) {
                //     new_node.tile_state = TileReadyToDraw;
                //     new_node.last_touched = new Date().getTime();
                //     that.ready_tiles_count++;
                //     that.midrequest_tiles_count--;
                //     on_success(new_node, data);
                // });
            }
        },
        clear: function(clean_node) {
            var old_max_size = max_size;
            max_size = 0;
            this.trim(clean_node);
            max_size = old_max_size;
        },
        trim: function(clean_node) {
            if (this.ready_tiles_count <= max_size) {
                return;
            }
            
            var times = [];
            var that = this;
            function collect_last_touched(node) {
                if (node.tile_state === TileReadyToDraw)
                    times.push(node.last_touched);
                return true;
            }
            this.root.level_visit(collect_last_touched);

            // this could be done by a linear-time select...
            times.sort();
            var excess_nodes = this.ready_tiles_count - max_size;
            var cutoff_time;
            if (excess_nodes >= times.length)
                cutoff_time = times[excess_nodes-1]+1;
            else
                cutoff_time = times[excess_nodes];

            function trim_recurse(node) {
                var must_exist;
                var remaining_count = 0;
                for (var i=0; i<4; ++i)
                    if (node.children[i]) {
                        must_exist = trim_recurse(node.children[i]);
                        if (!must_exist)
                            node.children[i] = undefined;
                        else
                            remaining_count += 1;
                    }
                
                if (node.tile_state === TileReadyToDraw && node.last_touched <= cutoff_time) {
                    node.tile_state = TileInactive;
                    that.ready_tiles_count--;
                    clean_node(node);
                }
                // don't purge the node
                // if it is still somewhat active, or if it still has useful children
                return (node.tile_state !== TileInactive) || (remaining_count > 0);
            }
            trim_recurse(this.root);
        }
    };
    return cache;
}
