
function applyElementUpdate(element, update) {
    if (update.hasOwnProperty("update-elements")) {
        var childUpdates = update["update-elements"];
        for (var i = 0; i < childUpdates.length; i++) {
            var childUpdate = childUpdates[i];
            var index = childUpdate["index"];
            var child = element.childNodes.item(index);
            applyElementUpdate(child, childUpdate);
        }
    }
    var removes = null;
    if (update.hasOwnProperty("remove")) {
        removes = [];
        var removeIndices = update["remove"];
        for (var i = 0; i < removeIndices.length; i++) {
            removes.push(element.childNodes.item(removeIndices[i]));
        }
    }
    var moves = null;
    if (update.hasOwnProperty("move")) {
        moves = [];
        var moveIndices = update["move"];
        for (var i = 0; i < moveIndices.length; i++) {
            moves.push(element.childNodes.item(moveIndices[i]));
        }
    }
    var insertions = null;
    var insertionPoints = null;
    if (update.hasOwnProperty("insert")) {
        insertions = [];
        insertionPoints = [];
        var descriptors = update["insert"];
        for (var i = 0; i < descriptors.length; i++) {
            var html = descriptors[i]["html"];
            var insertionIndex = descriptors[i]["index"];
            insertions.push(html);
            insertionPoints.push(element.childNodes.item(insertionIndex));
        }
    }

    if (update.hasOwnProperty("classes")) {
        // TODO(yjbanov): properly diff the class list.
        var classes = update['classes'];
        if (classes.length > 0) {
            element.className = "";
            for (var i = 0; i < classes.length; i++) {
                var className = classes[i];
                if (className != '__clear__') {
                    element.classList.add(className);
                }
            }
        }
    }

    if (removes != null) {
        for (var i = 0; i < removes.length; i++) {
            removes[i].remove();
        }
    }

    if (moves != null) {
        for (var i = 0; i < moves.length; i += 2) {
            element.insertBefore(moves[i + 1], moves[i]);
        }
    }

    if (insertions != null) {
        for (var i = 0; i < insertions.length; i++) {
            var template = document.createElement("template");
            template.innerHTML = insertions[i];
            element.insertBefore(template.content.firstChild, insertionPoints[i]);
        }
    }

    if (update.hasOwnProperty("bid")) {
        element.setAttribute("_bid", update["bid"]);
    }
    if (update.hasOwnProperty("text")) {
        element.innerText = update["text"];
    }
    if (update.hasOwnProperty("attrs")) {
        var attrs = update["attrs"];
        for (var name in attrs) {
            if (attrs.hasOwnProperty(name)) {
                element.setAttribute(name, attrs[name]);
            }
        }
    }
}
