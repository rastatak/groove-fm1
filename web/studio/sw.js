/* Hors ligne : l'app se garde en cache après la première visite, et se met à jour en arrière-plan */
const C='fm1groove-v30',F=['./','index.html','install.html','styles.css','fm1.css','data.js','audio.js','midi.js','lib.js','app.js','rdbpack.js','rdb.js','fm1.js','scenes.js','icons.js','manifest.webmanifest','icon-192.png','icon-512.png'];
self.addEventListener('install',e=>{e.waitUntil(caches.open(C).then(c=>c.addAll(F)).then(()=>self.skipWaiting()));});
self.addEventListener('activate',e=>{e.waitUntil(caches.keys().then(k=>Promise.all(k.filter(x=>x!==C).map(x=>caches.delete(x)))).then(()=>self.clients.claim()));});
self.addEventListener('fetch',e=>{if(e.request.method!=='GET')return;
  e.respondWith(caches.match(e.request).then(r=>{const n=fetch(e.request).then(res=>{if(res.ok){const cp=res.clone();caches.open(C).then(c=>c.put(e.request,cp));}return res;}).catch(()=>r);return r||n;}));});
