/* ---------- USB MIDI : le FM-1 branché à l'ordinateur ----------
   Entrée : les touches et les knobs du FM-1 pilotent l'app (vue FM-1) ou jouent la piste sélectionnée (éditeur).
   Sortie : chaque piste peut jouer sur les sons du FM-1 (canal au choix), horloge MIDI pour caler son séquenceur. */
const MIDI={acc:null,out:null,inId:'',held:{},learn:null,
  map:(()=>{try{return JSON.parse(localStorage.getItem('fm1groove.map'))||{};}catch(e){return{};}})(),
  base:+(localStorage.getItem('fm1groove.base')||48),kb:localStorage.getItem('fm1groove.kb')||'auto'};
const DRN=[36,38,39,42,46,63,41,37]; /* notes General MIDI de la batterie : kick, snare, clap, hat fermé, hat ouvert, conga, tom grave, rim */
const MOUT=['Interne','FM‑1','Les deux'];
const isFM=p=>/fm-?1|felucca|hortator|melodee|shrub|m-?vave|choral|omni/i.test(p&&p.name||'');
const tCh=i=>S.tracks[i].mc||i+1,dCh=()=>S.dmc||10;
// Batterie : chaque ligne a sa sortie, son canal et sa note (par défaut : ceux de toute la batterie)
const rCh=r=>S.rows[r].mc||dCh(),rOut=r=>S.rows[r].mo!=null?S.rows[r].mo:(S.dmo||0),rNote=r=>S.rows[r].mn||DRN[r];
// Horloge : int = l'app mène (et peut envoyer son horloge) · ext = l'app suit l'horloge reçue · auto = suit si une horloge arrive
const CLK={last:0,iv:[],cnt:0,bpm:0};const clkExt=()=>S.mcm==='ext'||S.mcm==='auto'&&performance.now()-CLK.last<1200;
// Heure MIDI (performance.now) correspondant à un temps de l'horloge audio
const mts=t=>performance.now()+Math.max(0,t-ctx.currentTime)*1000+((ctx.outputLatency||ctx.baseLatency||0)*1000);
const msend=(d,t)=>{try{MIDI.out.send(d,t==null?undefined:mts(t));}catch(e){}};
function midiOk(){return !!(MIDI.out&&ctx);}

async function midiConnect(quiet){
  if(MIDI.acc){midiPorts();return true;}
  if(!navigator.requestMIDIAccess){if(!quiet)msg('USB MIDI indisponible ici : ouvre l’app dans Chrome ou Edge sur ordinateur');midiUI();return false;}
  try{MIDI.acc=await navigator.requestMIDIAccess();}catch(e){if(!quiet)msg('Accès MIDI refusé : autorise le MIDI pour ce site (icône à gauche de l’adresse) puis réessaie');midiUI();return false;}
  MIDI.acc.onstatechange=()=>midiPorts();midiPorts();
  if(!quiet)msg(MIDI.out?'FM‑1 connecté en USB : '+MIDI.out.name:'MIDI ouvert, mais aucun appareil trouvé : branche le FM-1 en USB');
  return true;}
function midiPorts(){if(!MIDI.acc)return;const outs=[...MIDI.acc.outputs.values()],ins=[...MIDI.acc.inputs.values()],so=localStorage.getItem('fm1groove.out');
  MIDI.out=outs.find(p=>p.id===so)||outs.find(isFM)||outs[0]||null;
  const si=localStorage.getItem('fm1groove.in');MIDI.inId=si&&(si==='*'||ins.some(p=>p.id===si))?si:'*';
  ins.forEach(p=>{p.onmidimessage=p.id===MIDI.inId||MIDI.inId==='*'||MIDI.learn?midiIn:null;});
  midiUI();}

/* --- Sortie --- */
// Note d'une piste mélodique (pad = accord). Renvoie true si elle part sur le FM-1
function midiNoteOut(i,tr,d,t,dur,acc,gm){if(!midiOk())return false;const ch=tCh(i)-1,v=Math.max(1,Math.min(127,Math.round((acc?118:92)*(gm||1))));
  const ks=tr.type==='pad'?[0,...(SCP[S.scale]||SCP.Mineur).ch]:[0],ns=ks.map(k=>Math.max(0,Math.min(127,d2m(tr,d+k)+12*(tr.po||0))));
  if(MIDI.held[i]&&tr.type!=='pad')midiOff(i,t);
  ns.forEach(n=>msend([0x90|ch,n,v],t));
  if(dur>0)ns.forEach(n=>msend([0x80|ch,n,0],t+Math.max(.03,dur)));else MIDI.held[i]=ns;
  return true;}
function midiOff(i,t){const h=MIDI.held[i];if(!h||!midiOk())return;delete MIDI.held[i];h.forEach(n=>msend([0x80|(tCh(i)-1),n,0],t));}
function midiDrumOut(r,t,vel,gm){if(!midiOk())return false;const ch=rCh(r)-1,n=rNote(r),v=Math.max(1,Math.min(127,Math.round((vel===3?55:vel>1?120:90)*(gm||1))));
  msend([0x90|ch,n,v],t);msend([0x80|ch,n,0],t+.06);return true;}
// Horloge : 6 impulsions par double-croche (24 par noire)
function midiClock(t,sd){if(!S.mclk||clkExt()||!midiOk())return;for(let k=0;k<6;k++)msend([0xF8],t+k*sd/6);}
function midiRun(on,t){if(!midiOk())return;if(S.mclk&&!clkExt())msend([on?0xFA:0xFC],on?t:undefined);if(!on)midiPanic(true);}
function midiPanic(soft){if(!MIDI.out)return;MIDI.held={};for(let c=0;c<16;c++){msend([0xB0|c,123,0]);if(!soft)msend([0xB0|c,120,0]);}}
function midiTest(c){if(!MIDI.out)return msg('Connecte d’abord le FM-1');const ch=(c||(S.sel!=null?tCh(S.sel):1))-1;
  [0,4,7,12].forEach((k,j)=>{const n=60+k;MIDI.out.send([0x90|ch,n,100],performance.now()+j*160);MIDI.out.send([0x80|ch,n,0],performance.now()+j*160+140);});
  msg('Test envoyé au FM-1 sur le canal '+(ch+1)+' : tu dois entendre 4 notes');}

/* --- Entrée --- */
const mkey=(c,n)=>c+':'+n;
// Toute commande autre qu'une note : CC, pitch bend, program change, aftertouch → une clé et une valeur 0-127
function ctlKey(ty,c,a,b){if(ty===176)return[mkey(c,a),b];if(ty===224)return['pb'+c,b];if(ty===192)return['pc'+c,a];if(ty===208)return['at'+c,a];return null;}
const MON=[];
function monLog(d){if(d[0]>=0xF8)return;const ty=d[0]&240,c=(d[0]&15)+1;
  MON.unshift(ty===144&&d[2]?'note '+d[1]+' (canal '+c+')':ty===128||ty===144?'note relâchée '+d[1]:ty===176?'CC '+d[1]+' = '+d[2]+' (canal '+c+')':ty===224?'pitch bend '+d[2]:ty===192?'program change '+d[1]:d[0]===0xF0?'SysEx ('+d.length+' octets)':[...d].map(x=>x.toString(16)).join(' '));
  MON.length=Math.min(MON.length,5);const m=$('#lnMon');if(m)m.textContent=MON.join('\n');}
function clkIn(st){const now=performance.now();
  if(st===0xF8){if(CLK.last&&now-CLK.last<200){CLK.iv.push(now-CLK.last);if(CLK.iv.length>48)CLK.iv.shift();}CLK.last=now;CLK.cnt++;
    if(clkExt()&&CLK.iv.length>=24&&CLK.cnt%24===0){const av=CLK.iv.reduce((a,b)=>a+b,0)/CLK.iv.length,bpm=Math.round(60000/(av*24));CLK.bpm=bpm;
      if(bpm>=40&&bpm<=300&&Math.abs(bpm-S.bpm)>=1){S.bpm=Math.min(220,Math.max(60,bpm));const e=$('#bpm');if(e)e.value=S.bpm;applyParams();}}return;}
  if(S.mcm==='int')return;if(st===0xFA||st===0xFB){CLK.cnt=0;CLK.last=now;if(!playing)toggle();msg('Horloge externe : départ');}
  else if(st===0xFC){if(playing)toggle();msg('Horloge externe : arrêt');}}
function midiIn(ev){const[st,a,b]=ev.data,c=st&15,ty=st&240;if(st>=0xF8){clkIn(st);return;}monLog(ev.data);
  if(MIDI.learn){learnIn(ty,c,a,b);return;}
  if(ty===144||ty===128){const on=ty===144&&b>0;
    if(window.fm1Ext&&(MIDI.kb==='ctl'||MIDI.kb==='auto'&&window.VIEW==='fm1')){fm1Ext.note(a,on);return;}
    if(on){const sc=SCALES[S.scale],L=sc.length,x=a-S.root-12*(T().oct+1),o=Math.floor(x/12),r=((x%12)+12)%12;let k=0;sc.forEach((v,j)=>{if(v<=r)k=j;});liveOn(Math.max(0,o*L+k));}
    else liveOff();return;}
  const ck=ctlKey(ty,c,a,b);if(ck){const m=MIDI.map[ck[0]];if(m&&window.fm1Ext)fm1Ext.ctl(m.k,m.t,ck[1]);}}

/* --- Apprentissage guidé des knobs du FM-1 --- */
const LEARN=[['KEY','Appuie sur la touche du bas la plus à gauche de ton FM-1'],['KNOB1','Tourne KNOB1 vers la droite'],['KNOB2','Tourne KNOB2 vers la droite'],['KNOB3','Tourne KNOB3 vers la droite'],['KNOB4','Tourne KNOB4 vers la droite'],
  ['MASTER','Tourne MASTER vers la droite'],['SELECT','Tourne SELECT vers la droite'],['PRESETS','Tourne PRESETS vers la droite'],['ALGORITHM','Tourne ALGORITHM vers la droite']];
async function learnStart(){if(!await midiConnect())return;if(!MIDI.acc||![...MIDI.acc.inputs.values()].length)return msg('Aucune entrée MIDI : branche le FM-1 en USB');
  MIDI.learn={i:0,got:null,vals:[],t0:Date.now(),any:0};midiPorts();$('#learn').hidden=false;learnUI();clearInterval(MIDI.lt);MIDI.lt=setInterval(learnUI,500);}
function learnIn(ty,c,a,b){const L=MIDI.learn,k=LEARN[L.i][0];
  if(k==='KEY'){if(ty===144&&b){L.got={n:a};learnUI();}return;}
  const ck=ctlKey(ty,c,a,b);if(!ck)return;L.any++;const key=ck[0];if(!L.got||L.got.key!==key){L.got={key,c,a};L.vals=[];}L.vals.push(ck[1]);learnUI();}
// Valeurs reçues : toujours les mêmes (1, 65, 127…) = encodeur relatif ; sinon position absolue
function learnType(v){const u=[...new Set(v)];if(v.length>=3&&u.length<=2){if(u.every(x=>x>=56&&x<=72))return'r64';return'r2';}return'abs';}
function learnNext(skip){const L=MIDI.learn,k=LEARN[L.i][0];
  if(!skip&&L.got){if(k==='KEY'){MIDI.base=L.got.n;localStorage.setItem('fm1groove.base',MIDI.base);}
    else{for(const x in MIDI.map)if(MIDI.map[x].k===k)delete MIDI.map[x];MIDI.map[L.got.key]={k,t:learnType(L.vals)};}}
  L.i++;L.got=null;L.vals=[];L.t0=Date.now();
  if(L.i>=LEARN.length){localStorage.setItem('fm1groove.map',JSON.stringify(MIDI.map));learnEnd();msg('Knobs du FM-1 appris : ils pilotent maintenant la vue FM-1');return;}
  learnUI();}
function learnEnd(){MIDI.learn=null;clearInterval(MIDI.lt);$('#learn').hidden=true;midiPorts();}
function learnUI(){const L=MIDI.learn;if(!L)return;const[k,txt]=LEARN[L.i];
  $('#lnStep').textContent=(L.i+1)+' / '+LEARN.length;$('#lnTxt').textContent=txt;
  const wait=Date.now()-L.t0>5000&&!L.got&&k!=='KEY';
  $('#lnGot').textContent=!L.got?(wait?'Rien reçu de ce knob. Le firmware actuel du FM‑1 n’envoie pas ses knobs en USB (seulement les touches) : clique sur « Passer ». Sur la machine, les knobs marcheront avec le firmware Groove.':'En attente du FM‑1…'):k==='KEY'?'Reçu : note '+L.got.n:'Reçu : '+L.got.key.replace(/^(\d+):(\d+)$/,(m,c,n)=>'CC '+n+' (canal '+(+c+1)+')')+' · '+(learnType(L.vals)==='abs'?'position':'encodeur')+' · '+L.vals.slice(-6).join(' ');
  $('#lnGot').classList.toggle('wait',wait);
  $('#lnGot').classList.toggle('ok',!!L.got);$('#lnNext').disabled=!L.got;$('#lnNext').textContent=L.i===LEARN.length-1?'Terminer':'Suivant';}

/* --- Interface (éditeur + barre du haut) --- */
function midiUI(){const has=!!MIDI.acc,outs=has?[...MIDI.acc.outputs.values()]:[],ins=has?[...MIDI.acc.inputs.values()]:[],on=!!MIDI.out;
  document.querySelectorAll('.mconn').forEach(b=>{b.classList.toggle('on',on);b.textContent=on?'🔌 '+(isFM(MIDI.out)?'FM‑1':MIDI.out.name.slice(0,14))+' connecté':'🔌 Connecter le FM‑1';});
  if(!$('#mOut'))return;
  $('#mOut').innerHTML=outs.length?outs.map(p=>`<option value="${p.id}"${MIDI.out&&p.id===MIDI.out.id?' selected':''}>${esc(p.name)}</option>`).join(''):'<option>—</option>';
  $('#mIn').innerHTML=ins.length?ins.map(p=>`<option value="${p.id}"${p.id===MIDI.inId?' selected':''}>${esc(p.name)}</option>`).join('')+`<option value="*"${MIDI.inId==='*'?' selected':''}>Toutes les entrées</option>`:'<option>—</option>';
  $('#mOut').disabled=$('#mIn').disabled=!has;$('#mKb').value=MIDI.kb;$('#mClk').checked=!!S.mclk;$('#mClkM').value=S.mcm||'int';
  const nk=Object.keys(MIDI.map).length;
  $('#mSt').textContent=!has?'Non connecté':on?(isFM(MIDI.out)?'FM‑1 détecté':'Appareil MIDI')+' · '+(nk?nk+' knob(s) appris':'touches du FM‑1 actives (ses knobs n’envoient rien avec le firmware actuel)'):'Aucun appareil MIDI branché';
  const sel=(k,nm,p,v,c,n)=>`<div class="mrw" style="--cc:${trkC(p)}">${iconSVG(p<8?ROWCAT[p]:TYPCAT[S.tracks[p-8].type],18)}<b>${esc(nm)}</b><select data-mo="${k}" title="Où joue cette piste">${MOUT.map((l,j)=>`<option value="${j}"${j===(v||0)?' selected':''}>${l}</option>`).join('')}</select>`+
    `<label class="ck">Canal<input type="number" min="1" max="16" value="${c}" data-mc="${k}"></label>`+(n!=null?`<label class="ck">Note<input type="number" min="0" max="127" value="${n}" data-mn="${k}" title="Note MIDI envoyée pour ce son"></label>`:'')+`<button data-mtest="${c}" title="Jouer 4 notes sur ce canal">♪</button></div>`;
  $('#mRoute').innerHTML='<div class="mrh"><b>Batterie</b><span>toute la batterie :</span><select data-mo="d">'+MOUT.map((l,j)=>`<option value="${j}"${j===(S.dmo||0)?' selected':''}>${l}</option>`).join('')+`</select><label class="ck">Canal<input type="number" min="1" max="16" value="${dCh()}" data-mc="d"></label></div>`+
    '<div class="mrg">'+ROWS.map((n,r)=>sel('r'+r,n,r,rOut(r),rCh(r),rNote(r))).join('')+'</div><div class="mrh"><b>Mélodie</b></div><div class="mrg">'+S.tracks.map((t,i)=>sel('t'+i,t.name,8+i,t.mo,tCh(i))).join('')+'</div>';}
// Réglage MIDI d'une piste : k = 'd' (toute la batterie), 'r0'..'r7' (lignes), 't0'..'t5' (pistes mélodiques)
function midiSet(k,f,v){if(f==='mc')v=Math.min(16,Math.max(1,Math.round(v)||1));if(f==='mn')v=Math.min(127,Math.max(0,Math.round(v)||0));
  if(k==='d'){if(f==='mo'){S.dmo=v;S.rows.forEach(r=>delete r.mo);}else if(f==='mc'){S.dmc=v;S.rows.forEach(r=>delete r.mc);}}
  else if(k[0]==='r')S.rows[+k.slice(1)][f]=v;
  else{const i=+k.slice(1);if(f==='mo')midiOff(i,ctx?ctx.currentTime:0);if(f!=='mn')S.tracks[i][f]=v;}
  if(f==='mo'&&v&&!MIDI.out)msg('Connecte le FM-1 pour entendre cette piste sur lui');applyParams();save();}
function midiInit(){
  document.querySelectorAll('.mconn').forEach(b=>b.onclick=()=>{if(!MIDI.out)return midiConnect();if(window.setView)setView('ed');$('#midiS').scrollIntoView({behavior:'smooth'});});
  $('#mc2').onclick=()=>midiConnect();$('#mTest').onclick=midiTest;$('#mPanic').onclick=()=>{midiPanic();msg('Toutes les notes du FM-1 coupées');};
  $('#mLearn').onclick=learnStart;
  $('#mOut').onchange=e=>{midiPanic(true);localStorage.setItem('fm1groove.out',e.target.value);midiPorts();};
  $('#mIn').onchange=e=>{localStorage.setItem('fm1groove.in',e.target.value);midiPorts();};
  $('#mKb').onchange=e=>{MIDI.kb=e.target.value;localStorage.setItem('fm1groove.kb',MIDI.kb);};
  $('#mClk').onchange=e=>{S.mclk=e.target.checked?1:0;save();msg(S.mclk?'Horloge MIDI envoyée au FM-1 : son séquenceur et son arpège suivent le tempo':'Horloge MIDI coupée');};
  $('#mRoute').addEventListener('change',e=>{const d=e.target.dataset,k=d.mo||d.mc||d.mn;if(!k)return;
    midiSet(k,d.mo?'mo':d.mc?'mc':'mn',+e.target.value);midiUI();});
  $('#mRoute').addEventListener('click',e=>{const c=e.target.dataset.mtest;if(c)midiTest(+c);});
  $('#mClkM').onchange=e=>{S.mcm=e.target.value;save();msg('Horloge : '+{int:'interne (l’app mène le tempo)',ext:'externe (l’app suit l’horloge reçue)',auto:'auto (suit une horloge reçue, sinon interne)'}[S.mcm]);};
  $('#lnNext').onclick=()=>learnNext(false);$('#lnSkip').onclick=()=>learnNext(true);$('#lnStop').onclick=learnEnd;
  $('#lnReset').onclick=()=>{MIDI.map={};localStorage.removeItem('fm1groove.map');msg('Réglages des knobs effacés');midiUI();};
  midiUI();
  // Si le site a déjà la permission MIDI, on se reconnecte tout seul
  if(navigator.permissions&&navigator.requestMIDIAccess)navigator.permissions.query({name:'midi'}).then(p=>{if(p.state==='granted')midiConnect(true);}).catch(()=>{});}
