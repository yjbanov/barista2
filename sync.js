var Module = {};

function printStats() {
    console.log(document.querySelectorAll('*').length, 'elements');
    console.log(document.querySelectorAll('*[widget]').length, 'widgets');
}

function applyElementUpdate(element, update) {
    if (update.hasOwnProperty("update-elements")) {
        let childUpdates = update["update-elements"];
        for (let i = 0; i < childUpdates.length; i++) {
            let childUpdate = childUpdates[i];
            let index = childUpdate["index"];
            let child = element.childNodes.item(index);
            if (child == null) {
                console.log('Element child', index, 'not found in:');
                console.log(element);
                let parent = element.parentNode;
                while(parent.id != 'host') {
                    console.log(parent);
                    parent = parent.parentNode;
                }
            }
            applyElementUpdate(child, childUpdate);
        }
    }
    let removes = null;
    if (update.hasOwnProperty("remove")) {
        removes = [];
        let removeIndices = update["remove"];
        for (let i = 0; i < removeIndices.length; i++) {
            removes.push(element.childNodes.item(removeIndices[i]));
        }
    }
    let moves = null;
    if (update.hasOwnProperty("move")) {
        moves = [];
        let moveIndices = update["move"];
        for (let i = 0; i < moveIndices.length; i++) {
            moves.push(element.childNodes.item(moveIndices[i]));
        }
    }
    let insertions = null;
    let insertionPoints = null;
    if (update.hasOwnProperty("insert")) {
        insertions = [];
        insertionPoints = [];
        let descriptors = update["insert"];
        for (let i = 0; i < descriptors.length; i++) {
            let html = descriptors[i]["html"];
            let insertionIndex = descriptors[i]["index"];
            insertions.push(html);
            insertionPoints.push(element.childNodes.item(insertionIndex));
        }
    }

    if (update.hasOwnProperty("classes")) {
        // TODO(yjbanov): properly diff the class list.
        let classes = update['classes'];
        if (classes.length > 0) {
            element.className = "";
            for (let i = 0; i < classes.length; i++) {
                let className = classes[i];
                if (className != '__clear__') {
                    element.classList.add(className);
                }
            }
        }
    }

    if (removes != null) {
        for (let i = 0; i < removes.length; i++) {
            removes[i].remove();
        }
    }

    if (moves != null) {
        for (let i = 0; i < moves.length; i += 2) {
            element.insertBefore(moves[i + 1], moves[i]);
        }
    }

    if (insertions != null) {
        for (let i = 0; i < insertions.length; i++) {
            let template = document.createElement("template");
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
        let attrs = update["attrs"];
        for (let name in attrs) {
            if (attrs.hasOwnProperty(name)) {
                element.setAttribute(name, attrs[name]);
            }
        }
    }
}


function printPerf(category, start, end) {
    console.log('>>>', category, ':', end - start, 'ms');
}

let compileStart = 0;

function enteredMain() {
    let compileEnd = performance.now();
    printPerf('time to main', compileStart, compileEnd);
}

function allReady() {
    console.timeStamp('In main');
    let renderFrame = Module.cwrap('RenderFrame', 'string', []);
    let dispatchEvent = Module.cwrap('DispatchEvent', 'void', ['string', 'string']);
    let host = document.querySelector('#host');

    function syncFromNative() {
        console.log('>>> ====== syncFromNative =======');
        console.timeStamp('Start frame build');
        let renderStart = performance.now();
        let json = renderFrame();
        console.timeStamp('End frame build');
        setTimeout(() => {
          let renderFullEnd = performance.now();
          printPerf('renderFull', renderStart, renderFullEnd);
        }, 0);
        let renderEnd = performance.now();
        printPerf('renderFrame', renderStart, renderEnd);
        console.log('>>> diff size: ', json.length);

        let jsonParseStart = performance.now();
        let diff = JSON.parse(json);
        if (!diff) {
            return;
        }
        console.timeStamp('Parse diff');
        let jsonParseEnd = performance.now();
        printPerf('parseJson', jsonParseStart, jsonParseEnd);

        if (diff.hasOwnProperty("create")) {
            let createStart = performance.now();
            host.innerHTML = diff["create"];
            let createEnd = performance.now();
            printPerf('create', createStart, createEnd);
        } else if (diff.hasOwnProperty("update")) {
            let updateStart = performance.now();
            applyElementUpdate(host.firstElementChild, diff["update"]);
            let updateEnd = performance.now();
            printPerf('update', updateStart, updateEnd);
        }
        console.timeStamp('End apply diff');
    }

    host.addEventListener("click", function(event) {
        // Look for the nearest parent with a _bid, then dispatch to it.
        let bid = null;
        let parent = event.target;
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

function openDB() {
  return new Promise(function(resolve, reject) {
    var dbOpen = indexedDB.open("WasmCache");
    dbOpen.onerror = function(event) {
      reject("Failed to open IndexedDB 'WasmCache'");
    };
    dbOpen.onupgradeneeded = function(event) {
      var db = event.target.result;
      db.createObjectStore("modules", { keyPath: "name" });
    }
    dbOpen.onsuccess = function(event) {
      var db = event.target.result;
      db.onerror = function(event) {
        alert("Database error: " + event.target.errorCode);
      };
      resolve(db);
    };
  });
}

function loadModuleFromIndexedDB() {
  return new Promise(function(resolve, reject) {
    openDB().then(function(db) {
      var transaction = db.transaction(["modules"], "readwrite");
      var objectStore = transaction.objectStore("modules");
      var getRequest = objectStore.get("giant.wasm");
      getRequest.onerror = function(event) {
        console.log('[ERROR] Failed to load module from IDB', event);
        reject(event);
      };
      getRequest.onsuccess = function(event) {
        if (getRequest.result) {
          resolve(getRequest.result.module);
        } else {
          resolve(null);
        }
      };
    });
  });
}

function storeModuleInIndexedDB(module) {
  return new Promise(function(resolve, reject) {
    openDB().then(function(db) {
      var transaction = db.transaction(["modules"], "readwrite");
      var objectStore = transaction.objectStore("modules");
      var addRequest = objectStore.put({name: "giant.wasm", module: module});
      addRequest.onerror = function(event) {
        console.log('[ERROR] Failed to store module in IDB', event);
        reject(event);
      };
      addRequest.onsuccess = function(event) {
        if (addRequest.result) {
          resolve(addRequest.result.module);
        } else {
          resolve(null);
        }
      };
    });
  });
}

function loadModuleFromNetwork(fileNameWithoutExtension) {
  return fetch(`${fileNameWithoutExtension}.wasm`)
      .then(function (response) {
        return response.arrayBuffer();
      })
      .then(function (bytes) {
        var beforeCompile = performance.now();
        return WebAssembly.compile(bytes).then(function(module) {
          var afterCompile = performance.now();
          console.log('>>> wasm compile', afterCompile - beforeCompile);
          return module;
        });
      });
}

let loadStart = null;

function bootstrapWasm(fileNameWithoutExtension, module, bytes) {
  if (module) {
    Module['module'] = module;
  } else if (bytes) {
    Module['wasmBinary'] = bytes;
  }
  let script = document.createElement('script');
  script.src = `${fileNameWithoutExtension}.js`;
  document.body.appendChild(script);
  let loadEnd = performance.now();
  printPerf('load', loadStart, loadEnd);
}

function loadApp(fileNameWithoutExtension) {
  if(window.location.hash && window.location.hash.indexOf('idb') != -1) {
    loadAppWithIDB(fileNameWithoutExtension);
  } else {
    loadAppWithoutIDB(fileNameWithoutExtension);
  }
}

function loadAppWithoutIDB(fileNameWithoutExtension) {
  loadStart = performance.now();
  console.log('>>> loading from network');
  return fetch(`${fileNameWithoutExtension}.wasm`)
      .then(function (response) {
        return response.arrayBuffer();
      })
      .then(function (bytes) {
        bootstrapWasm(fileNameWithoutExtension, null, bytes);
      });
}

function loadAppWithIDB(fileNameWithoutExtension) {
  loadStart = performance.now();
  loadModuleFromIndexedDB().then(function(module) {
    if (module) {
      loadIdb = performance.now();
      console.log('>>> loaded from IndexedDB in ', loadIdb - loadStart, 'ms');
      bootstrapWasm(fileNameWithoutExtension, module);
    } else {
      console.log('>>> loading from network');
      loadModuleFromNetwork(fileNameWithoutExtension).then(function(module) {
        storeModuleInIndexedDB(module).then(function(_) {
          bootstrapWasm(fileNameWithoutExtension, module);
        });
      });
    }
  });
}
