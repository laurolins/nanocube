# Example

Insert [[1],[1],[1]], [[2],[2],[1]], [[1],[2],[3]] into a 3
dimensional NC with compression. In the image below we show the final
result after inserting these three paths into the NC. Circle node
annotations indicate the path of labels from the parent to that
node. An annotation "e" means an empty label path. In general we could
have node annotations like 1,1 or 1,2,3. In this example, though,
since each hierarchy in each level has height 1, only empty (e) or one
label annotations (e.g. 1 or 2 or 3) are present. Blue arrows are
called content links, solid black segments are called parent-child
links, and dashed gray links are called shared parent-child links.

![example-1](/doc/compressed-nc-example-1.png)