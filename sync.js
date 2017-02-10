var Module = {};

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


function printPerf(category, start, end) {
    console.log('>>>', category, ':', end - start, 'ms');
}

var compileStart = 0;

function enteredMain() {
    var compileEnd = performance.now();
    printPerf('time to main', compileStart, compileEnd);
}

function allReady() {
    var renderFrame = Module.cwrap('RenderFrame', 'string', []);
    var dispatchEvent = Module.cwrap('DispatchEvent', 'void', ['string', 'string']);
    var host = document.querySelector('#host');

    function syncFromNative() {
        console.log('>>> ====== syncFromNative =======');
        var renderStart = performance.now();
        var json = renderFrame();
        console.log(json);
        var renderEnd = performance.now();
        printPerf('renderFrame', renderStart, renderEnd);
        console.log('>>> diff size: ', json.length);

        var jsonParseStart = performance.now();
        var diff = JSON.parse(json);
        if (!diff) {
            return;
        }
        var jsonParseEnd = performance.now();
        printPerf('parseJson', jsonParseStart, jsonParseEnd);

        if (diff.hasOwnProperty("create")) {
            var createStart = performance.now();
            host.innerHTML = diff["create"];
            var createEnd = performance.now();
            printPerf('create', createStart, createEnd);
        } else if (diff.hasOwnProperty("update")) {
            var updateStart = performance.now();
            applyElementUpdate(host.firstElementChild, diff["update"]);
            var updateEnd = performance.now();
            printPerf('update', updateStart, updateEnd);
        }
    }

    host.addEventListener("click", function(event) {
        // Look for the nearest parent with a _bid, then dispatch to it.
        var bid = null;
        var parent = event.target;
        while(bid == null && parent && parent != document) {
            bid = parent.getAttribute("_bid");
            parent = parent.parentNode;
        }
        if (bid) {
            dispatchEvent("click", bid);
            syncFromNative();
        } else {
            console.log(">>> caught event on target with no _bid:", event.target);
        }
    });
    syncFromNative();
}

function loadApp(fileNameWithoutExtension) {
    var loadStart = performance.now();
    fetch(`${fileNameWithoutExtension}.wasm`)
        .then(function (response) {
            return response.arrayBuffer();
        })
        .then(function (bytes) {
            var loadEnd = performance.now();
            printPerf('load', loadStart, loadEnd);

            compileStart = performance.now();
            Module['wasmBinary'] = bytes;
            var script = document.createElement('script');
            script.src = `${fileNameWithoutExtension}.js`;
            document.body.appendChild(script);
        });
}
