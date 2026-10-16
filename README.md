# Delivery & Logistics Hub

## Files by team member
| Member | Backend (C++) | Frontend (browser) |
| --- | --- | --- |
| Ashutosh | `Parcel.h`, `Hub.h`, `Trie.h`, `Json.h`, `HubManager.h`, `HttpServer.h`, `main.cpp` | `index.html`, `api.js`, `app.js` |
| Prachi | `MinHeap.h`, `HashTable.h` | `track.js` |
| Vikalp | `Containers.h` (circular queue, stack) | `belt.js` |
| Swastika | `Route.h` (doubly linked list) | `register.js`, `route.js`, `style.css` |

## Run it
Needs only a C++17 compiler (g++). Run these from this folder.

Linux / macOS:
    make
    ./hub

Windows (MinGW, PowerShell):
    g++ -std=c++17 -Iashutosh -Iprachi -Ivikalp -Iswastika ashutosh/main.cpp -o hub.exe -lws2_32
    .\hub.exe

Then open http://localhost:3000 in your browser. Use `hub 4000` to pick another port.

No backend yet? Double-click `ashutosh/index.html`. Opened as a file it runs in mock mode with fake data.

## How the frontend reaches the backend
Browser JS (`api.js`) sends POST /api/<name> with a JSON body. `HttpServer.h` receives it, `main.cpp` passes it to
`HubManager::handle`, which calls the data structures and replies with JSON. Errors come back as `{"error": "..."}`.

| Endpoint | Body | Returns | Data structure used |
| --- | --- | --- | --- |
| /api/suggest | prefix | list of pin and name | Trie |
| /api/quote | service, weight, distance | cost | Strategy pattern |
| /api/register | pincode, service, weight, distance | parcel | Hash table + min-heap |
| /api/track | id | parcel | Hash table |
| /api/dispatch | none | parcel | Min-heap + circular queue |
| /api/loadVan | none | count, top | Circular queue + stack (loads in reverse so top priority unloads first) |
| /api/deliver | none | id | Stack |
| /api/cancelStop | name | name | Doubly linked list (O(1) unlink) |
| /api/state | none | parcels, belt, van, route | all |

## Notes
- The web server is written by hand on sockets, so the project still has zero external libraries.
- The server listens on localhost only, and serves only .html, .css and .js files.
- Pincodes and place names in `HubManager.h` are sample placeholders.
- Parcel status is a simple enum for now. Swastika replaces it with the State pattern in the 40% phase.
