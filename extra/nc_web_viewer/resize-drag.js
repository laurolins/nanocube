interact('.resize-drag')
    .draggable({
        // enable inertial throwing
        inertia: false,
        
        // keep the element within the area of it's parent
        restrict: {
            restriction: "parent",
            endOnly: true,
            elementRect: { top: 0, left: 0, bottom: 1, right: 1 }
        },
        
        // enable autoScroll
        autoScroll: true,
        
        // call this function on every dragmove event
        onmove: dragMoveListener
    })
    .resizable({
        preserveAspectRatio: false,
        edges: { left: true, right: true, bottom: true, top: true }
    })
    .on('resizemove', resizeMoveListener);

function resizeMoveListener(event) {
    var target = event.target,
        x = (parseFloat(target.getAttribute('data-x')) || 0),
        y = (parseFloat(target.getAttribute('data-y')) || 0);
    
    // update the element's style
    target.style.width  = event.rect.width + 'px';
    target.style.height = event.rect.height + 'px';
    
    // translate when resizing from top or left edges
    x += event.deltaRect.left;
    y += event.deltaRect.top;
    
    target.style.webkitTransform = target.style.transform =
        'translate(' + x + 'px,' + y + 'px)';
    
    target.setAttribute('data-x', x);
    target.setAttribute('data-y', y);

    //fire an event...
    var e = new Event('divresize',target);
    target.dispatchEvent(e);
}

function dragMoveListener (event) {
    var target = event.target,
        // keep the dragged position in the data-x/data-y attributes
        x = (parseFloat(target.getAttribute('data-x')) || 0) + event.dx,
        y = (parseFloat(target.getAttribute('data-y')) || 0) + event.dy;
    
    if (target.classList.contains("brushing")){
        return;
    }
    
    // translate the element
    target.style.webkitTransform =
        target.style.transform =
        'translate(' + x + 'px, ' + y + 'px)';
    
    // update the posiion attributes
    target.setAttribute('data-x', x);
    target.setAttribute('data-y', y);
}
