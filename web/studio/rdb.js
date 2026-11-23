/* BASE DE RYTHMES : tes motifs de batterie importés en MIDI (fichiers .mid, ou un .zip qui en contient), gardés dans le
 * navigateur, à part du projet. « Rythme random » et l'auto-random génèrent comme avant, ou piochent dans cette base
 * (réglage « Random pioche dans »). Les fichiers restent chez toi : rien n'est envoyé ni publié. */
let RDB=[];   /* (tes motifs importés ; RDBP = ceux du pack intégré, non effaçables) */
const RDBP=(typeof RDB_PACK!=='undefined'?RDB_PACK:[]).map((e,k)=>({id:'pk'+k,pk:1,genre:e.genre,name:e.name,bpm:e.bpm,g:e.g.split(' ').map(r=>[...r].map(Number))})),rdbAll=()=>[...RDBP,...RDB];
const rdbFits=e=>(typeof RDB_G2S!=='undefined'&&RDB_G2S[e.genre]||[e.st]).includes(S.style);
const RDBK='fm1groove-rdb';
/* carte General MIDI → nos 8 lignes (KICK SNARE CLAP HAT F. HAT O. PERC LOW RIM), et l'inverse pour l'export */
const GM2ROW=n=>n===35||n===36?0:n===38||n===40?1:n===39?2:n===37||n===75||n===76||n===77?7:n===42||n===44?3:n===46||n===49||n===52||n===55||n===57?4
  :n===41||n===43||n===45||n===47||n===48||n===50?6:n>=27&&n<=87?5:-1;
const ROW2GM=[36,38,39,42,46,70,45,37],VEL=[0,92,112,60];
/* genre du fichier → un de nos styles (pour piocher « dans le style en cours ») ; sinon le genre garde son nom */
const GENRE2ST={'techno':'techno','melodic techno':'techno','hard techno':'schranz','minimal techno':'minimal','minimal':'minimal','trance':'trance','psytrance':'psytrance',
  'trap':'trap','drill':'drill','house':'house','deep house':'house','tech house':'house','progressive house':'house','bass house':'house','nu disco':'house','edm':'house',
  'boom bap':'hip-hop','hip-hop':'hip-hop','hip hop':'hip-hop','lo-fi':'hip-hop','acid':'acid','acid house':'acid house','hardcore':'hardcore','dub':'dub'};
async function rdbLoad(){try{const d=window.deskosia?await deskosia.store.get('rdb'):JSON.parse(localStorage.getItem(RDBK));if(Array.isArray(d))RDB=d.filter(e=>e&&Array.isArray(e.g)&&e.g.length===8);}catch(e){}
  rdbRender();}
async function rdbSave(){try{if(window.deskosia)await deskosia.store.set('rdb',RDB);else localStorage.setItem(RDBK,JSON.stringify(RDB));}catch(e){msg('Base de rythmes : sauvegarde impossible ('+e.message+')');}}

/* ---------- lecture d'un fichier MIDI : notes (tick, note, vélocité), tempo, marqueurs ---------- */
function midiParse(buf){const d=new Uint8Array(buf);if(d.length<22||String.fromCharCode(d[0],d[1],d[2],d[3])!=='MThd')throw new Error('pas un fichier MIDI');
  const u16=p=>d[p]<<8|d[p+1],u32=p=>(d[p]*16777216)+(d[p+1]<<16|d[p+2]<<8|d[p+3]),nt=u16(10),div=u16(12);if(div&0x8000||!div)throw new Error('MIDI en temps SMPTE : non géré');
  const ev=[],mk=[];let bpm=0,pos=8+u32(4);
  for(let k=0;k<nt&&pos+8<=d.length;k++){const len=u32(pos+4);let q=pos+8,end=Math.min(d.length,q+len),t=0,rs=0;
    const vl=()=>{let v=0,c;do{c=d[q++];v=v*128+(c&127);}while(c&128&&q<end);return v;};
    while(q<end){t+=vl();const b=d[q];
      if(b===0xFF){const ty=d[q+1];q+=2;const l=vl();if(ty===0x51&&l===3&&!bpm)bpm=Math.round(60000000/(d[q]<<16|d[q+1]<<8|d[q+2]));
        else if(ty===6||ty===3){let s='';for(let i=0;i<l;i++)s+=String.fromCharCode(d[q+i]);try{s=decodeURIComponent(escape(s));}catch(e){}if(ty===6)mk.push([t,s]);}q+=l;}
      else if(b===0xF0||b===0xF7){q++;q+=vl();}
      else{if(b&128){rs=b;q++;}const hi=rs&0xF0,a=d[q],v=hi===0xC0||hi===0xD0?0:d[q+1];q+=hi===0xC0||hi===0xD0?1:2;if(hi===0x90&&v>0)ev.push([t,a,v]);}}
    pos+=8+len;}
  return{div,bpm,ev,mk};}
/* un fichier → un motif par mesure (16 pas), les mesures identiques une seule fois ; vélocité : la plus forte = accent, les très douces = fantôme */
function midiToPatterns(buf,name,genre){const m=midiParse(buf),notes=[...new Set(m.ev.map(e=>e[1]))];
  if(!m.ev.length)return[];if(notes.length===1&&notes[0]===60)return[];   /* (« un fichier par instrument », tout sur do : le motif complet est déjà dans le pack) */
  const st=m.div/4,vmax=Math.max(...m.ev.map(e=>e[2])),lv=new Set(m.ev.map(e=>e[2])).size>1,bars={};
  m.ev.forEach(([t,n,v])=>{const r=GM2ROW(n);if(r<0)return;const s=Math.round(t/st),b=Math.floor(s/16),c=s%16,g=bars[b]||(bars[b]=ROWS.map(()=>Array(16).fill(0)));
    const val=lv&&v>=vmax-6?2:v<vmax*.6?3:1;g[r][c]=g[r][c]===2||val===2?2:g[r][c]===1||val===1?1:val;});
  const out=[],seen=new Set(),keys=Object.keys(bars).map(Number).sort((a,b)=>a-b);
  keys.forEach(b=>{const g=bars[b],sig=JSON.stringify(g);if(seen.has(sig))return;seen.add(sig);
    const mark=m.mk.find(x=>Math.round(x[0]/st/16)===b),mm=mark&&mark[1].split(' / ');
    out.push({name:(mm?mm[mm.length-1]:name).slice(0,40),genre:(mm&&mm.length>1?mm[0]:genre)||'Divers',bpm:m.bpm||0,g});});
  if(out.length>1&&!m.mk.length)out.forEach((e,k)=>{e.name=(e.name.slice(0,34)+' · '+(k+1)).slice(0,40);});
  return out;}
/* un motif (ou toute la base, une mesure par motif avec son nom en marqueur) → fichier MIDI, canal 10, carte General MIDI */
function midiBuild(list,bpm){const vl=v=>{const o=[v&127];while(v>>=7)o.unshift((v&127)|128);return o;},tx=s=>[...unescape(encodeURIComponent(s))].map(c=>c.charCodeAt(0)),T=120;
  const ev=[],us=Math.round(60000000/(bpm||120));let abs=[];
  abs.push([0,0,[0xFF,0x51,3,us>>16&255,us>>8&255,us&255]]);
  list.forEach((e,b)=>{const nm=tx((e.genre?e.genre+' / ':'')+e.name);abs.push([b*16*T,1,[0xFF,6,...vl(nm.length),...nm]]);
    e.g.forEach((row,r)=>row.forEach((v,c)=>{if(!v)return;const t=(b*16+c)*T;abs.push([t,3,[0x99,ROW2GM[r],VEL[v]||92]]);abs.push([t+T-10,2,[0x89,ROW2GM[r],0]]);}));});
  abs.sort((a,b)=>a[0]-b[0]||a[1]-b[1]);let last=0;abs.forEach(([t,,by])=>{ev.push(...vl(t-last),...by);last=t;});
  ev.push(...vl(list.length*16*T-last),0xFF,0x2F,0);
  const u32=n=>[n>>>24&255,n>>>16&255,n>>>8&255,n&255];
  return new Uint8Array([0x4D,0x54,0x68,0x64,0,0,0,6,0,0,0,1,T*4>>8,T*4&255,0x4D,0x54,0x72,0x6B,...u32(ev.length),...ev]);}

/* ---------- un .zip : ses fichiers .mid (lecture du répertoire, décompression par le navigateur) ---------- */
async function zipMidis(buf){const d=new Uint8Array(buf),dv=new DataView(buf),out=[];let e=d.length-22;
  while(e>=0&&dv.getUint32(e,true)!==0x06054b50)e--;if(e<0)throw new Error('zip illisible');
  let p=dv.getUint32(e+16,true);const n=dv.getUint16(e+10,true);
  for(let k=0;k<n&&dv.getUint32(p,true)===0x02014b50;k++){const meth=dv.getUint16(p+10,true),cs=dv.getUint32(p+20,true),nl=dv.getUint16(p+28,true),xl=dv.getUint16(p+30,true),cl=dv.getUint16(p+32,true),lo=dv.getUint32(p+42,true);
    let path=new TextDecoder().decode(d.subarray(p+46,p+46+nl));p+=46+nl+xl+cl;
    if(!/\.midi?$/i.test(path))continue;
    const ds=lo+30+dv.getUint16(lo+26,true)+dv.getUint16(lo+28,true),raw=d.subarray(ds,ds+cs);let data;
    if(meth===0)data=raw.slice().buffer;
    else if(meth===8&&window.DecompressionStream)data=await new Response(new Blob([raw]).stream().pipeThrough(new DecompressionStream('deflate-raw'))).arrayBuffer();
    else continue;
    out.push({path,data});}
  return out;}
const rdbGenre=path=>{const a=path.replace(/\\/g,'/').split('/').filter(Boolean),f=a[a.length-1].replace(/\.midi?$/i,'');
  let g=a.length>1?a[a.length-2]:'';if(/one file per drum/i.test(g))g=a.length>2?a[a.length-3]:'';
  if(!g||/midi|pattern|pack/i.test(g)){const m=f.match(/^(.*?)\s+(drum pattern|basic beat|beat|pattern|drums?)\b/i);g=m?m[1]:'Divers';}
  return{genre:g.trim().slice(0,24)||'Divers',name:f.replace(/\s*\d+\s*BPM\s*/i,' ').replace(/\s+/g,' ').trim()};};
async function rdbImport(files){let add=0,skip=0,bad=0;const have=new Set(rdbAll().map(e=>e.genre+JSON.stringify(e.g)));   /* (le même motif dans deux genres : gardé dans les deux) */
  const one=(buf,path)=>{try{const{genre,name}=rdbGenre(path),L=midiToPatterns(buf,name,genre);if(!L.length){skip++;return;}
      L.forEach(e=>{const sig=e.genre+JSON.stringify(e.g);if(have.has(sig)){skip++;return;}have.add(sig);e.id='r'+Date.now().toString(36)+Math.random().toString(36).slice(2,6);
        e.st=GENRE2ST[e.genre.toLowerCase()]||(STYLES[e.genre.toLowerCase()]?e.genre.toLowerCase():'');RDB.push(e);add++;});}catch(err){bad++;}};
  for(const f of files){const buf=await f.arrayBuffer(),path=f.webkitRelativePath||f.name;
    if(/\.zip$/i.test(f.name)){try{for(const z of await zipMidis(buf))one(z.data,z.path);}catch(err){bad++;}}else one(buf,path);}
  RDB.sort((a,b)=>a.genre.localeCompare(b.genre)||a.name.localeCompare(b.name));await rdbSave();rdbRender();
  msg(add?add+' motif'+(add>1?'s':'')+' ajouté'+(add>1?'s':'')+' à la base de rythmes'+(skip?' · '+skip+' déjà là ou sans batterie':'')+(bad?' · '+bad+' fichier(s) illisible(s)':'')
    :'Aucun nouveau motif'+(bad?' : '+bad+' fichier(s) illisible(s)':skip?' : déjà dans la base, ou pas de batterie General MIDI dedans':''));}

/* ---------- la base à l'écran, et la pioche ---------- */
const rdbFilt=()=>{const f=S&&S.rgen||'',A=rdbAll();return f?A.filter(e=>e.genre===f):A;};
function rdbRender(){const el=document.getElementById('rdbl');if(!el||!S)return;const A=rdbAll(),gs=[...new Set(A.map(e=>e.genre))];
  if(S.rgen&&!gs.includes(S.rgen))S.rgen='';
  $('#rdbg').innerHTML='<option value="">Tous les genres ('+A.length+')</option>'+gs.map(g=>`<option value="${esc(g)}">${esc(g)} (${A.filter(e=>e.genre===g).length})</option>`).join('');$('#rdbg').value=S.rgen||'';
  $('#rsrc').value=S.rsrc||'gen';
  const L=rdbFilt();
  el.innerHTML=A.length?L.map(e=>`<span class="mchip${e.pk?' pk':''}"><button class="chip" data-rdb="${e.id}" title="${esc(e.genre)}${e.bpm?' · '+e.bpm+' BPM':''} — clic : charger dans la matrice">${S.rgen?'':esc(e.genre)+' · '}${esc(e.name)}${e.bpm?' <small>'+e.bpm+'</small>':''}</button>${e.pk?'':`<button class="chip x" data-rdbx="${e.id}" title="Enlever de la base">✕</button>`}</span>`).join('')
    :'<small>La base est vide. « Importer MIDI / ZIP » accepte des fichiers .mid de batterie (carte General MIDI) ou un .zip qui en contient ; chaque mesure différente devient un motif.</small>';}
/* un motif de la base pour « Rythme random » / l'auto-random : le genre choisi dans la liste ; sinon ceux du style en cours ; sinon toute la base */
function rdbPick(){const A=rdbAll();if(!A.length)return null;let L=rdbFilt();if(!(S.rgen||'')){const m=A.filter(rdbFits);if(m.length)L=m;}
  const now=JSON.stringify(cur().g),c=L.filter(e=>JSON.stringify(e.g)!==now);return c.length?pick(c):L.length?pick(L):null;}
function rdbGo(e,setBpm){const r={name:(e.genre+' · '+e.name).slice(0,24),tmp:true,st:setBpm?e.st||S.style:S.style,g:e.g.map(r=>r.slice())};
  if(setBpm&&e.bpm)r.bpm=e.bpm;   /* (chargé à la main : son tempo ; pioché par le random : le tempo et le style en cours restent) */
  go(r,true);}
function rdbDown(bytes,name){const a=document.createElement('a');a.href=URL.createObjectURL(new Blob([bytes],{type:'audio/midi'}));a.download=name;document.body.appendChild(a);a.click();setTimeout(()=>{URL.revokeObjectURL(a.href);a.remove();},500);}
function rdbInit(){
  $('#rdbi').onclick=()=>$('#rdbf').click();
  $('#rdbf').onchange=async e=>{const f=[...e.target.files];e.target.value='';if(f.length)await rdbImport(f);};
  $('#rdbg').onchange=e=>{S.rgen=e.target.value;rdbRender();save();};
  $('#rsrc').onchange=e=>{S.rsrc=e.target.value;save();msg(S.rsrc==='db'?'Random et auto-random piochent dans la base de rythmes'+'':S.rsrc==='mix'?'Random et auto-random : une fois la génération, une fois la base':'Random et auto-random : génération, comme avant');};
  $('#rdba').onclick=async()=>{const g=cur().g.map(r=>r.slice()),sig=JSON.stringify(g);if(rdbAll().some(e=>JSON.stringify(e.g)===sig))return msg('Ce rythme est déjà dans la base');
    const gn=S.rgen||S.style[0].toUpperCase()+S.style.slice(1);RDB.push({id:'r'+Date.now().toString(36),name:cur().name.slice(0,40),genre:gn,st:S.style,bpm:S.bpm,g});
    RDB.sort((a,b)=>a.genre.localeCompare(b.genre)||a.name.localeCompare(b.name));await rdbSave();rdbRender();msg('« '+cur().name+' » ajouté à la base de rythmes ('+gn+')');};
  $('#rdbx').onclick=()=>{rdbDown(midiBuild([{name:cur().name,genre:'',g:cur().g}],S.bpm),(cur().name||'rythme').replace(/[^\w\- ]+/g,'_')+' '+S.bpm+' BPM.mid');msg('Rythme exporté en MIDI (1 mesure, canal 10, General MIDI)');};
  $('#rdbxa').onclick=()=>{const L=rdbFilt();if(!L.length)return msg('La base est vide');rdbDown(midiBuild(L,S.bpm),'Base de rythmes'+(S.rgen?' '+S.rgen:'')+'.mid');msg(L.length+' motif(s) exporté(s) dans un fichier MIDI : une mesure par motif, son nom en marqueur');};
  $('#rdbl').addEventListener('click',async e=>{const d=e.target.closest('[data-rdb],[data-rdbx]');if(!d)return;
    if(d.dataset.rdbx){RDB=RDB.filter(x=>x.id!==d.dataset.rdbx);await rdbSave();rdbRender();return;}
    const x=rdbAll().find(x=>x.id===d.dataset.rdb);if(x){rdbGo(x,true);msg('Base : '+x.genre+' · '+x.name+(x.bpm?' · '+x.bpm+' BPM':'')+(playing?' — à la prochaine mesure':''));}});
  rdbLoad();}
(function wait(){if(typeof S!=='undefined'&&S&&document.getElementById('rdbi'))rdbInit();else setTimeout(wait,50);})();   /* (dès que le projet est chargé) */
