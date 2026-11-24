/* La génération de la machine dans le Studio : le firmware Groove lui-même (fm1sim.wasm, celui du simulateur) tire
   les rythmes et les mélodies, donc le Studio sort exactement ce que sort le FM-1. Sans le fichier : la génération web. */
let FW=null,fwErr='';
const fwOn=()=>!!FW&&(S.gen||'fw')==='fw';
const fwIdx=(o,k)=>Math.max(0,Object.keys(o).indexOf(k));
function fwCfg(){FW.gx_cfg(fwIdx(STYLES,S.style),S.root||0,fwIdx(SCALES,S.scale),S.prog||0,0,S.msrc==='base'?1:0);}
const fwMem=()=>new Uint8Array(FW.memory.buffer,FW.gx_buf(),8*64+64*5);
function fwRows(n){const m=fwMem();return Array.from({length:n},(_,b)=>Array.from({length:8},(_,r)=>Array.from({length:16},(_,c)=>{const v=m[r*64+b*16+c];return v&1?(v&2?2:1):0;})));}
function fwBars(n){const m=fwMem();return Array.from({length:n},(_,b)=>Array.from({length:16},(_,c)=>{const o=512+(b*16+c)*5;return{n:m[o]===255?-1:m[o],a:m[o+3],s:m[o+4],o:m[o+2]/4,l:Math.max(.25,m[o+1]/4)};}));}
/* un rythme du style : le modèle que le firmware a tiré, sur 1, 2 ou 4 mesures */
function fwRhythm(){try{fwCfg();const n=S.rbars||1,k=FW.gx_rhythm(n),bars=fwRows(n),t=STYLES[S.style].t[k-1];
  return{name:S.style[0].toUpperCase()+S.style.slice(1)+' · '+(t?t.split('|')[0]:'Machine'),tmp:true,st:S.style,g:bars[0],bars,eb:0};}catch(e){FW=null;return null;}}
function fwRow(r){try{fwCfg();const g=cur().g;FW.gx_row(r,g[r].reduce((a,v,c)=>a|(v?1<<(15-c):0),0));return fwRows(1)[0][r];}catch(e){FW=null;return null;}}
function fwMel(i){try{if(i>5)return null;fwCfg();const tr=S.tracks[i],n=tr.nb||(tr.bars?tr.bars.length:1);FW.gx_mel(i,n);return fwBars(n);}catch(e){FW=null;return null;}}
function fwVary(i){try{if(i>5)return null;fwCfg();const tr=fixT(S.tracks[i]),n=tr.bars.length,m=fwMem();
  tr.bars.forEach((st,b)=>st.forEach((s,c)=>{const o=512+(b*16+c)*5;m[o]=s.n<0?255:s.n;m[o+1]=Math.max(1,Math.min(64,Math.round((s.l||1)*4)));m[o+2]=Math.round((s.o||0)*4)&3;m[o+3]=s.a?1:0;m[o+4]=s.s?1:0;}));
  for(let c=n*16;c<64;c++)m[512+c*5]=255;
  FW.gx_vary(i,n,tr.eb||0);return fwBars(n);}catch(e){FW=null;return null;}}
function fwUI(){const e=document.getElementById('gsrc'),s=document.getElementById('gsrcs');if(e){e.value=FW?(S.gen||'fw'):'web';e.disabled=!FW;}
  if(s)s.textContent=FW?(fwOn()?'identique à la machine':'génération du Studio'):'firmware introuvable'+(fwErr?' ('+fwErr+')':'')+' : génération du Studio';}
(async()=>{for(const u of['fm1sim.wasm','../fm1sim.wasm']){try{const r=await fetch(u);if(!r.ok){fwErr='fichier absent';continue;}
    const mod=await WebAssembly.compile(await r.arrayBuffer()),imp={};
    for(const q of WebAssembly.Module.imports(mod))if(q.kind==='function')(imp[q.module]??={})[q.name]=()=>0;
    const x=(await WebAssembly.instantiate(mod,imp)).exports;if(!x.gx_boot){fwErr='firmware trop ancien';continue;}
    if(!x.gx_boot((Math.random()*4294967295)>>>0||1)){fwErr='démarrage';continue;}
    FW=x;fwErr='';break;}catch(e){fwErr='chargement';}}
  const go=()=>{if(typeof S==='undefined'||!S)return setTimeout(go,200);fwUI();};go();})();
