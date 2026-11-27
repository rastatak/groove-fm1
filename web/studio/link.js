/* Liaison USB Studio ↔ FM-1 Groove : le morceau, le rythme et les mélodies passent par le câble, en SysEx, dans les
   trames de l’éditeur du firmware (F0 7D 48 54 cmd seq … F7 ; firmware : editor_groove.c et groove_link.c).
   0x30 INFO · 0x31 MORCEAU · 0x32/0x33 RYTHME lire/écrire · 0x34/0x35 MÉLODIE lire/écrire. Tous les octets < 128. */
const LK={acc:null,out:null,inp:null,seq:0,pend:null,ok:false,busy:false,last:{},t:0,err:''};
const lkP16=v=>[v&127,(v>>7)&127,(v>>14)&3],lkG16=(a,o)=>a[o]|a[o+1]<<7|(a[o+2]&3)<<14;
/* ---- les octets (les mêmes que groove_link.c) ---- */
function lkSong(){const b=Math.max(40,Math.min(240,Math.round(S.bpm||120)));return[fwIdx(STYLES,S.style),b&127,b>>7,S.root||0,fwIdx(SCALES,S.scale),Math.min(S.prog||0,(PROGS[S.scale]||[]).length)];}
function lkRyt(){const r=fixR(cur()),n=r.bars.length>=4?4:r.bars.length>=2?2:1,o=[n];
  for(let b=0;b<n;b++)for(let k=0;k<8;k++){let on=0,ac=0;(r.bars[b][k]||[]).forEach((v,c)=>{if(v&&c<16){on|=1<<(15-c);if(v===2)ac|=1<<(15-c);}});o.push(...lkP16(on),...lkP16(ac));}
  return o;}
function lkRytIn(a){const n=a[0],bars=[];let k=1;
  for(let b=0;b<n;b++){const g=[];for(let r=0;r<8;r++,k+=6){const on=lkG16(a,k),ac=lkG16(a,k+3);g.push(Array.from({length:16},(_,c)=>on>>(15-c)&1?(ac>>(15-c)&1?2:1):0));}bars.push(g);}
  return bars;}
function lkMel(i){const tr=fixT(S.tracks[i]),n=tr.bars.length>=4?4:tr.bars.length>=2?2:1,o=[i,n];
  for(let b=0;b<n;b++)for(let c=0;c<16;c++){const s=tr.bars[b][c]||{n:-1};
    o.push(s.n<0?127:Math.min(28,s.n),Math.max(1,Math.min(64,Math.round((s.l||1)*4))),(Math.round((s.o||0)*4)&3)|(s.a?4:0)|(s.s?8:0));}
  return o;}
function lkMelIn(a){const n=a[1],bars=[];
  for(let b=0;b<n;b++)bars.push(Array.from({length:16},(_,c)=>{const o=2+(b*16+c)*3;return{n:a[o]===127?-1:a[o],a:a[o+2]>>2&1,s:a[o+2]>>3&1,o:(a[o+2]&3)/4,l:Math.max(.25,a[o+1]/4)};}));
  return bars;}
/* ---- le câble ---- */
function lkIn(ev){const d=ev.data,p=LK.pend;if(!p||d.length<8||d[0]!==0xF0||d[1]!==0x7D||d[2]!==0x48||d[3]!==0x54||d[4]!==p.cmd||d[5]!==p.seq)return;
  LK.pend=null;clearTimeout(p.to);const rc=d[6],body=Array.from(d.slice(7,d.length-1));
  if(rc)p.no(new Error(String.fromCharCode(...body.slice(1).filter(x=>x>=32))||'refusé (code '+rc+')'));else p.ok(body);}
function lkReq(cmd,args){return new Promise((ok,no)=>{if(!LK.out)return no(new Error('pas connecté'));
  const seq=LK.seq=(LK.seq+1)%127,fr=[0xF0,0x7D,0x48,0x54,cmd,seq,...args,0xF7];let tries=0;
  const go=()=>{if(tries++>=3){LK.pend=null;return no(new Error('la machine ne répond pas'));}
    LK.pend={cmd,seq,ok,no,to:setTimeout(go,1200)};try{LK.out.send(fr);}catch(e){clearTimeout(LK.pend.to);LK.pend=null;no(e);}};go();});}
function lkPorts(){if(!LK.acc)return;const f=l=>[...l.values()].find(p=>/hortator/i.test(p.name||''))||[...l.values()].find(p=>/fm-?1|m-?vave/i.test(p.name||''));
  const o=f(LK.acc.outputs),i=f(LK.acc.inputs);if(LK.inp&&LK.inp!==i)LK.inp.removeEventListener('midimessage',lkIn);
  if(i&&LK.inp!==i)i.addEventListener('midimessage',lkIn);LK.out=o||null;LK.inp=i||null;if(!o||!i){LK.ok=false;LK.last={};}}
async function lkConnect(){LK.err='';
  if(!navigator.requestMIDIAccess){LK.err='USB indisponible ici : ouvre le Studio dans Chrome ou Edge (ordinateur ou Android)';lkUI();return false;}
  try{LK.acc=await navigator.requestMIDIAccess({sysex:true});}catch(e){LK.err='Accès refusé : autorise « périphériques MIDI » pour ce site, puis réessaie';lkUI();return false;}
  LK.acc.onstatechange=()=>{lkPorts();lkUI();};lkPorts();
  if(!LK.out||!LK.inp){LK.err='FM-1 introuvable : branche-le en USB et allume-le';lkUI();return false;}
  try{const r=await lkReq(0x30,[]);if(r[0]!==71||r[1]!==86)throw new Error('ce firmware n’a pas la liaison Groove');LK.ok=true;LK.info=r.slice(3);}
  catch(e){LK.ok=false;LK.err=/GROOVE/.test(e.message)?'La machine est du côté Hortator : repasse en Groove':'Pas de réponse : installe le dernier firmware Groove ('+e.message+')';}
  lkUI();if(LK.ok)msg('FM-1 Groove relié par USB');return LK.ok;}
/* ---- envoyer / recevoir ---- */
async function lkRun(f,what){if(LK.busy)return;if(!LK.ok&&!await lkConnect())return msg(LK.err);LK.busy=true;lkUI();
  try{await f();if(what)msg(what);}catch(e){msg('Liaison FM-1 : '+e.message);if(/répond pas|connecté/.test(e.message)){LK.ok=false;LK.err=e.message;}}LK.busy=false;lkUI();}
const lkKey=a=>a.join(',');
async function lkPut(k,cmd,a,force){const s=lkKey(a);if(!force&&LK.last[k]===s)return 0;await lkReq(cmd,a);LK.last[k]=s;return 1;}
async function lkPutAll(force){let n=0;n+=await lkPut('s',0x31,lkSong(),force);n+=await lkPut('r',0x33,lkRyt(),force);
  for(let i=0;i<6&&i<S.tracks.length;i++)n+=await lkPut('t'+i,0x35,lkMel(i),force);return n;}
const lkSendAll=()=>lkRun(()=>lkPutAll(true),'Envoyé au FM-1 : tempo, gamme, rythme et les 6 mélodies');
const lkSendRyt=()=>lkRun(async()=>{await lkPut('s',0x31,lkSong(),true);await lkPut('r',0x33,lkRyt(),true);},'Rythme envoyé au FM-1');
const lkSendMel=()=>lkRun(async()=>{if(S.sel>5)throw new Error('la machine n’a que 6 pistes mélodiques');await lkPut('s',0x31,lkSong(),true);await lkPut('t'+S.sel,0x35,lkMel(S.sel),true);},'Mélodie envoyée au FM-1');
const lkPull=()=>lkRun(async()=>{const s=await lkReq(0x30,[]),bpm=s[4]|s[5]<<7,bars=lkRytIn(await lkReq(0x32,[]));
  for(let i=0;i<6&&i<S.tracks.length;i++){const b=lkMelIn(await lkReq(0x34,[i])),tr=S.tracks[i];tr.bars=b;tr.nb=b.length;tr.eb=0;tr.steps=b[0];delete LP['t'+i];}
  if(bpm>=40&&bpm<=240){S.bpm=Math.min(220,Math.max(60,bpm));$('#bpm').value=S.bpm;applyParams();}
  S.rbars=bars.length;const r={name:'FM-1 · reçu',tmp:true,st:S.style,g:bars[0],bars,eb:0};S.rhythms.push(r);applyR(r);renderRoll();LK.last={};save();},'Reçu du FM-1 : rythme, mélodies et tempo');
/* « Suivre » : chaque changement du Studio part vers la machine, sans rien renvoyer deux fois */
function lkAuto(){if(!S.lkauto||!LK.ok||LK.busy)return;clearTimeout(LK.t);LK.t=setTimeout(()=>{if(S.lkauto&&LK.ok&&!LK.busy)lkRun(()=>lkPutAll(false));},350);}
function lkUI(){const e=document.getElementById('lkSt');if(!e)return;document.getElementById('lkC').textContent=LK.ok?'🔗 FM‑1 Groove relié':'🔗 Relier le FM‑1 Groove';
  e.textContent=LK.busy?'envoi…':LK.ok?'relié · la machine reçoit tempo, gamme, rythme et mélodies':LK.err||'pas relié';
  ['lkAll','lkR','lkM','lkP'].forEach(i=>{const b=document.getElementById(i);if(b)b.disabled=LK.busy;});const a=document.getElementById('lkA');if(a)a.checked=!!(S&&S.lkauto);}
function lkInit(){const on=(i,f)=>{const b=document.getElementById(i);if(b)b.onclick=f;};on('lkC',()=>lkConnect());on('lkAll',lkSendAll);on('lkR',lkSendRyt);on('lkM',lkSendMel);on('lkP',lkPull);
  const a=document.getElementById('lkA');if(a)a.onchange=()=>{S.lkauto=a.checked;save();if(S.lkauto){LK.last={};lkAuto();}};lkUI();}
