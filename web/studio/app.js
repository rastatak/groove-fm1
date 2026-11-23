let S,playing=false,step=0,nextT=0,timer,bar=0,queued=null,curStep=-1,rec=false,saveT,LP={},fxR=0,mdel=-1,impT=false,liveI=-1,cix=0,rvI=-1,rvS=0,rvR=-2,rvDS=0;
const $=s=>document.querySelector(s),rnd=n=>Math.floor(Math.random()*n),pick=a=>a[rnd(a.length)];
const esc=s=>String(s).replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));
const msg=t=>{$('#msg').textContent=t;};
const cur=()=>S.rhythms[S.cur],T=()=>S.tracks[S.sel],SL=()=>SCALES[S.scale].length;
/* LIVE : effets tenus (LV), piste audible ou non (solo, coupure de groupe, mute), groupe visé par les effets */
let LV={lp:0,rl:0,rv:0},lvH=null,latch=false,pvD=0,pvQ=0;
const aud=(k,i)=>!S.gm[k]&&(S.solo.length?S.solo.includes(k+i):!(k==='r'?S.rows:S.tracks)[i].mute);
const lv=k=>S.lt&&S.lt!==k?{}:LV;
const KEYS=['KeyA','KeyS','KeyD','KeyF','KeyG','KeyH','KeyJ','KeyK','KeyL','Semicolon','KeyQ','KeyW','KeyE','KeyR','KeyT'];
const GUESS=[[0,/kick|bd|bassdrum/i],[1,/snare|sd/i],[2,/clap|cp/i],[4,/open|oh/i],[3,/hat|hh|closed/i],[5,/perc|conga|bongo|shaker/i],[6,/tom|low|sub/i],[7,/rim|clave|stick/i]];
const FX=[['vol','Volume'],['cut','Cutoff'],['res','Réso'],['env','Env'],['att','Attaque'],['dec','Decay'],['gl','Glide'],['lfr','LFO vitesse'],['lfd','LFO prof.'],['dist','Disto'],['dly','Delay'],['rev','Reverb']];
const pv=v=>'Pitch '+(v>0?'+':'')+(v||0)+' oct';
const LPS=[0,1,2,3,4,6,8],RLS=[1,2,3,4];
// Boutons Loop / Roll (piste mélodique ou ligne de batterie)
function seg(o,noRoll){const b=(k,n,v,l)=>`<button data-${k}="${n}"${v===n?' class="on"':''}>${l}</button>`;
  return `<div class="lbl" title="Boucle les N pas à partir de la dernière note jouée">Loop (pas)<span class="seg">${LPS.map(n=>b('lp',n,o.lp||0,n||'Off')).join('')}</span></div>`+(noRoll?'':`<div class="lbl" title="Rejoue la note plusieurs fois dans le pas, en écho">Roll<span class="seg">${RLS.map(n=>b('rl',n,o.rl||1,n>1?'×'+n:'Off')).join('')}</span></div>`);}

async function doSave(){saveT=0;try{if(window.deskosia)await deskosia.store.set('projet',S);else localStorage.setItem('fm1groove',JSON.stringify(S));}catch(e){msg('Sauvegarde impossible : '+e.message);}}
function save(){clearTimeout(saveT);saveT=setTimeout(doSave,700);}
// Fermeture ou rechargement de la page : on enregistre tout de suite ce qui attendait
['pagehide','beforeunload'].forEach(ev=>window.addEventListener(ev,()=>{if(saveT){clearTimeout(saveT);doSave();}}));
document.addEventListener('visibilitychange',()=>{if(document.hidden&&saveT){clearTimeout(saveT);doSave();}});

/* ---------- Affichage ---------- */
const AUS=[0,4,8,16,32,64,128];
function renderPads(){const P=(k,i,n,m,x,a)=>`<div class="pd${aud(k,i)?'':' dim'}" style="--kc:${trkC(k==='r'?i:8+i)}"><button class="pad${x}${m?' off':''}" data-m${k}="${i}">${n}</button><input type="range" class="vf" min="0" max="1" step="0.01" value="${(k==='r'?S.rows:S.tracks)[i].vol}" data-pv="${k+i}" title="Volume de cette piste : glisse pour l’amener ou la retirer en douceur"><button class="sb${S.solo.includes(k+i)?' on':''}" data-so="${k+i}" title="Solo : n’écouter que cette piste (plusieurs solos possibles)">S</button><div class="aur"><select class="au${a?' on':''}" data-au="${k+i}" title="Auto-random : ${k==='r'?'nouveau motif pour cette ligne':'nouvelle mélodie pour cette piste'} toutes les N mesures">${AUS.map(q=>`<option value="${q}"${q===(a||0)?' selected':''}>${q?'Auto '+q:'Auto off'}</option>`).join('')}</select>${k==='t'?`<button class="rb" data-rm="${i}" title="Nouvelle mélodie pour cette piste, tout de suite (dans la gamme et sur les accords en cours)">🎲</button>`:''}</div></div>`;
  if(pvD)pvQ=1;else{$('#pads').innerHTML=ROWS.map((n,r)=>P('r',r,n+`<small>${r+1}</small>`,S.rows[r].mute,'',S.rows[r].ar)).join('');
  $('#padsT').innerHTML=$('#padsM').innerHTML=S.tracks.map((t,i)=>P('t',i,t.name,t.mute,' t',t.ar)).join('');}
  document.querySelectorAll('[data-gm]').forEach(b=>{const k=b.dataset.gm;b.classList.toggle('on',!!S.gm[k]);b.textContent=(S.gm[k]?'🔊 Relancer ':'🔇 Couper ')+(k==='r'?'tout le rythme':'toute la mélodie');});
  $('#soOff').disabled=$('#soOff2').disabled=!S.solo.length;
  document.querySelectorAll('[data-lt]').forEach(b=>b.classList.toggle('on',b.dataset.lt===(S.lt||'')));}
function renderBank(){$('#bank').innerHTML=S.rhythms.map((r,k)=>`<button class="chip${k===S.cur?' on':''}${r===queued?' q':''}${r.tmp?' tmp':''}" data-rk="${k}">${k+1} · ${esc(r.name)}${r.tmp?' 🎲':''}</button>`).join('');
  if(document.activeElement!==$('#rname'))$('#rname').value=cur().name;}
function renderRhythm(){renderBank();const g=cur().g;
  $('#drums').innerHTML=ROWS.map((nm,r)=>{const row=S.rows[r];
    return `<div class="rw" style="--rc:${trkC(r)}"><button class="mt${row.mute?' off':''}${row.lp||row.rl>1?' fx':''}" data-mr="${r}">${nm}</button><select data-sr="${r}">${smpOpts(row.smp,'Son interne',ROWF[r])}</select><input type="range" class="v" min="0" max="1" step="0.01" value="${row.vol}" data-vr="${r}">`+g[r].map((v,c)=>`<i data-c="${c}" data-r="${r}" class="${v?(v===2?'n ac':v===3?'n gh':'n'):''}${c%4?'':' b'}"></i>`).join('')+'</div>';}).join('');
  renderPads();renderDfx();}
function renderDfx(){const all=fxR<0,same=k=>S.rows.every(r=>r[k]===S.rows[0][k]),
    o=all?{lp:same('lp')?S.rows[0].lp:0,rl:same('rl')?S.rows[0].rl:1}:S.rows[fxR];
  $('#dfx').innerHTML=`<label title="Ligne visée par Loop, Roll et Reverse">Effet sur<select id="fxRow"><option value="-1"${all?' selected':''}>★ TOUTES LES LIGNES</option>${ROWS.map((n,r)=>`<option value="${r}"${r===fxR?' selected':''}>${n}${S.rows[r].lp||S.rows[r].rl>1?' ●':''}</option>`).join('')}</select></label>`+seg(o)+`<div class="lbl" title="Maintiens le bouton (ou la touche C) : la ligne choisie dans « Effet sur » (ou toute la batterie) joue à l’envers · relâche : retour à la normale">Reverse (touche C)<button id="drev"${rvR>-2?' class="on"':''}>◀◀ Maintenir</button></div>`;}
function renderTabs(){$('#tabs').innerHTML=S.tracks.map((t,i)=>`<button class="tab${i===S.sel?' on':''}${t.mute?' off':''}" style="--kc:${trkC(8+i)}" data-st="${i}">${t.name}${t.bw?' ◀':''}${t.ar?' 🎲':''}${t.smp==='piano'?' 🎹':t.smp?' ♪':''}</button>`).join('');}
function renderRoll(){$('#roll').style.setProperty('--tc',trkC(8+S.sel));const tr=T(),sc=SCALES[S.scale],L=sc.length,tl=tails(tr);let h='';
  for(let d=2*L;d>=0;d--){h+=`<div class="rw${d%L?'':' root'}"><span class="lb">${NOTES[(S.root+sc[d%L])%12]}</span>`;
    for(let c=0;c<16;c++)h+=`<i data-c="${c}" data-d="${d}" class="${ncls(tr,tl,c,d)}" style="${nsty(tr,tl,c,d)}"></i>`;
    h+='</div>';}
  [['a','ACCENT'],['s','SLIDE']].forEach(([k,l])=>{h+=`<div class="rw mod"><span class="lb">${l}</span>`;for(let c=0;c<16;c++)h+=`<i data-c="${c}" data-m="${k}" class="${tr.steps[c][k]?'m':''}${c%4?'':' b'}"></i>`;h+='</div>';});
  $('#roll').innerHTML=h;
  $('#fx').innerHTML=`<label>Onde<select data-p="wave">${[['sawtooth','Scie'],['square','Carré'],['triangle','Triangle']].map(([v,l])=>`<option value="${v}"${tr.wave===v?' selected':''}>${l}</option>`).join('')}</select></label><label>Octave<input type="number" data-p="oct" min="1" max="6" value="${tr.oct}"></label>`+`<label>Filtre<select data-p="ftype">${[['lowpass','Passe-bas'],['highpass','Passe-haut'],['bandpass','Passe-bande']].map(([v,l])=>`<option value="${v}"${(tr.ftype||'lowpass')===v?' selected':''}>${l}</option>`).join('')}</select></label>`+
  `<label>Forme LFO<select data-p="lfw">${['Sinus','Triangle','Carré','Scie'].map((l,j)=>`<option value="${j}"${(tr.lfw||0)===j?' selected':''}>${l}</option>`).join('')}</select></label><label>LFO calé<select data-p="lfs"><option value="0"${tr.lfs?'':' selected'}>Libre</option><option value="1"${tr.lfs?' selected':''}>Sur le tempo</option></select></label>`+FX.map(([k,l])=>`<label>${l}<input type="range" min="0" max="1" step="0.01" data-p="${k}" value="${tr[k]==null?(k==='lfr'?.3:0):tr[k]}"></label>`).join('')+`<label title="Octaves entières, toujours dans la gamme · double-clic = retour à 0"><span id="pv">${pv(tr.po)}</span><input type="range" min="-3" max="3" step="1" data-p="po" value="${tr.po||0}"></label>`+seg(tr,tr.type==='pad')+`<div class="lbl" title="Maintiens le bouton (ou la touche X) : la piste joue à l’envers · relâche : retour à la normale">Reverse (touche X)<button id="rev"${rvI===S.sel?' class="on"':''}>◀◀ Maintenir</button></div>`+`<div class="lbl" title="Sens de lecture du motif de cette piste : de droite à gauche tant que le bouton est allumé (le son reste normal) · reclique pour revenir à gauche → droite">Sens de lecture (touche V)<button id="bw"${tr.bw?' class="on"':''}>${tr.bw?'◀ Droite → gauche':'▶ Gauche → droite'}</button></div>`;renderSnd();}
// Son importé de la piste mélodique sélectionnée (basse, synthé, nappe, FX)
function renderSnd(){const tr=T();
  $('#tsmp').innerHTML=smpOpts(tr.smp,'Synthé interne',TYPF[tr.type]);
  $('#tsn').innerHTML=NOTES.map((n,i)=>`<option value="${i}"${i===(tr.sn||0)?' selected':''}>${n}</option>`).join('');
  $('#tsn').disabled=$('#tss').disabled=!tr.smp||tr.smp==='piano';$('#tss').value=tr.ss||0;$('#mauto').value=tr.ar||0;}
function setSmp(id){const tr=T(),was=tr.smp,l=id==='piano'?{name:'Piano'}:S.lib.find(x=>x.id===id);tr.smp=l?id:'';
  if(l){tr.so=tr.oct;tr.sn=S.root;tr.ss=0;if(!was)Object.assign(tr,{cut:1,res:.1,env:0,dist:0});}
  if(ctx)off(S.sel,ctx.currentTime);renderTabs();renderRoll();applyParams();save();
  if(id==='piano')return msg('Piano interne sur '+tr.name+' — « Decay » règle la durée de sa résonance');
  msg(l?'Son « '+l.name+' » sur '+tr.name+' — filtre ouvert ; règle « Note du son » si ce n’est pas juste':'Synthé interne sur '+tr.name);}
function renderKeys(){const sc=SCALES[S.scale],L=sc.length;let h='';for(let d=0;d<=2*L;d++)h+=`<button class="key${d%L?'':' r'}" data-k="${d}">${NOTES[(S.root+sc[d%L])%12]}</button>`;$('#keys').innerHTML=h;}
// Banque de mélodies enregistrées (clic = charger sur la piste sélectionnée)
function renderMels(){$('#mbank').innerHTML=S.mels.length?S.mels.map((m,k)=>`<span class="mchip"><button class="chip" data-ml="${k}" title="Charger sur la piste sélectionnée">▶ ${esc(m.name)}</button><button class="chip x${k===mdel?' sure':''}" data-mx="${k}" title="Supprimer">${k===mdel?'Supprimer ?':'✕'}</button></span>`).join(''):'<small>Aucune mélodie enregistrée : compose une piste puis clique sur 💾.</small>';}
// Pas affiché (et enregistré en REC) dans la grille mélodie : en lecture inversée, le curseur va de droite à gauche
const rst=()=>curStep<0?-1:T().bw?15-curStep:curStep;
let stT=0;
function hl(st){if(!playing)st=-1;curStep=st;stT=ctx?ctx.currentTime:0;document.querySelectorAll('.pl').forEach(e=>e.classList.remove('pl'));if(st>=0)document.querySelectorAll('#drums i[data-c="'+st+'"],#roll i[data-c="'+rst()+'"]').forEach(e=>e.classList.add('pl'));}

/* ---------- Banque de rythmes ---------- */
function go(r,add){if(add)S.rhythms.push(r);if(playing){queued=r;renderBank();}else applyR(r);}
// Style d'une matrice (mémorisé à la création, sinon déduit du nom « Tekno · … »)
const styleOf=r=>r&&(r.st||Object.keys(STYLES).find(k=>String(r.name).toLowerCase().startsWith(k+' ·')))||'';
// Change de style : le tempo passe automatiquement au BPM du style
function setStyle(s){if(!STYLES[s])return;S.style=s;$('#style').value=s;$('#lstyle').value=s;S.bpm=STYLES[s].bpm;$('#bpm').value=S.bpm;applyParams();
  msg('Style '+s[0].toUpperCase()+s.slice(1)+' : tempo réglé sur '+S.bpm+' BPM');}
// Gamme associée au style choisi dans un menu « Style » : la gamme change et les notes posées sont recalées (même chemin que le menu « Gamme »)
function styleSc(){const sc=STYLES[S.style].sc,e=$('#scale');if(!sc||!SCALES[sc])return;if(sc!==S.scale&&e.oninput){e.value=sc;e.oninput();}
  msg('Style '+S.style[0].toUpperCase()+S.style.slice(1)+' : '+S.bpm+' BPM · gamme '+sc);}
function applyR(r){const s=styleOf(r);if(s&&s!==S.style&&s!==styleOf(cur()))setStyle(s);
  if(r.bpm&&r.bpm!==S.bpm){S.bpm=r.bpm;$('#bpm').value=S.bpm;applyParams();}   /* (un motif de la base chargé à la main : son tempo) */
  if(r.tmp)S.rhythms=S.rhythms.filter(x=>!x.tmp||x===r);S.cur=Math.max(0,S.rhythms.indexOf(r));queued=null;ROWS.forEach((_,k)=>delete LP['r'+k]);renderRhythm();save();}
function randomRhythm(){
  /* la source : génération (les modèles du style), la base de rythmes (MIDI importés), ou une fois l’une une fois l’autre */
  if(S.rsrc==='db'||(S.rsrc==='mix'&&(S.rmix=!S.rmix))){const e=typeof rdbPick==='function'&&rdbPick();if(e)return rdbGo(e,false);}
  const list=STYLES[S.style].t;let p;do{p=pick(list);}while(list.length>1&&cur().name.endsWith(p.split('|')[0]));
  const[nm,hex]=p.split('|');go({name:S.style[0].toUpperCase()+S.style.slice(1)+' · '+nm,tmp:true,st:S.style,g:grid(hex)},true);}

// Auto-random d'une seule ligne de batterie : elle prend le motif de cette ligne dans un autre modèle (du style en cours si possible)
function randomRow(r){const cs=l=>l.map(p=>grid(p.split('|')[1])[r]).filter(a=>a.some(x=>x)),g=cur().g,now=g[r].map(x=>x?1:0).join(''),dif=a=>a.join('')!==now;
  let c=cs(STYLES[S.style].t).filter(dif);if(!c.length)c=cs(Object.values(STYLES).flatMap(s=>s.t)).filter(dif);if(!c.length)return;
  g[r]=pick(c);delete LP['r'+r];
  document.querySelectorAll('#drums i[data-r="'+r+'"]').forEach(e=>{const cc=+e.dataset.c;e.className=(g[r][cc]?'n':'')+(cc%4?'':' b');});save();}

/* ---------- Mélodie aléatoire dans la gamme ---------- */
/* Accords : un accord par mesure, toutes les pistes mélodiques le suivent ensemble */
function prg(){return S.prog&&(PROGS[S.scale]||[])[S.prog-1]||null;}
// Nom de l'accord construit sur le degré r de la gamme (ex. Lam, Fa, Sol)
function chName(r){const sc=SCALES[S.scale],L=sc.length,sem=d=>sc[d%L]+12*Math.floor(d/L),t=sem(r+2)-sem(r),f=sem(r+4)-sem(r);
  return NOTES[(S.root+sc[r%L])%12]+(L!==7?'':f===6?'°':f===8?'+':t===3?'m':'');}
// Décalage (en degrés) de l'accord de la mesure en cours : les accords hauts sont joués sous la tonique
function chOff(){const p=prg();if(!p||!playing)return 0;const r=p[cix]||0,L=SL();return r>L/2?r-L:r;}
function showCh(){const p=prg();$('#chd').textContent=p?'· '+chName(p[playing?cix:0]||0):'';}
function renderProg(){const P=PROGS[S.scale]||[];S.prog=Math.min(S.prog||0,P.length);
  $('#prog').innerHTML='<option value="0">Off · tonique fixe</option>'+P.map((p,k)=>`<option value="${k+1}"${k+1===S.prog?' selected':''}>${p.map(chName).join(' – ')}</option>`).join('');showCh();}
// Générateur : rythmes calés sur les 16 pas + notes de l'accord (les degrés sont relatifs à l'accord de la mesure)
function randomMel(i){const tr=S.tracks[i],L=SL(),ty=tr.type,P=SCP[S.scale]||SCP.Mineur,[t3,t5]=P.ch,
    st=Array.from({length:16},()=>({n:-1,a:0,s:0,o:0,l:1})),psy=ty==='bass'&&S.style==='psytrance';
  if(ty==='pad'){ /* nappe : l'accord tenu sur la mesure, ou sur deux demi-mesures */
    if(Math.random()<.35){st[0].n=0;st[0].l=8;st[8].n=pick([0,t3]);st[8].l=8;}else{st[0].n=0;st[0].l=16;}}
  else if(ty==='fx'){ /* FX : une à trois notes de l'accord, posées sur les temps */
    [0,4,8,12,6,14].sort(()=>Math.random()-.5).slice(0,1+rnd(3)).forEach(c=>{st[c].n=pick([L,L+t3,L+t5,t5]);st[c].l=pick([1,2,2]);});}
  else{
    /* rythme droit ou syncopé selon la gamme, toujours sur la grille */
    const R=RH[ty==='bass'?'bass':'synth'],rh=psy?'.xxx.xxx.xxx.xxx':pick(R[Math.random()<Math.min(.7,P.sy*(ty==='bass'?1:1.6))?'b':'a']),
      hits=[...rh].map((x,c)=>x==='x'?c:-1).filter(c=>c>=0),last=hits.length-1,
      CT=[0,t3,t5,L,L+t3,L+t5],near=x=>CT.reduce((b,v)=>Math.abs(v-x)<Math.abs(b-x)?v:b),
      arp=ty!=='bass'&&Math.random()<.5&&pick([[0,t3,t5,L],[0,t5,L,t5],[L,t5,t3,0],[0,t3,t5,L,t5,t3],[0,L,t5,L,t3,L],[0,t5,L+t3,L]]),al=pick([.5,.75,1]);
    let d=pick([t3,t5,L]);
    hits.forEach((c,k)=>{const s=st[c],f=(k<last?hits[k+1]:16)-c;
      if(ty==='bass')s.n=psy?(Math.random()<.9?0:L):(!k||c%8===0)?0:pick([0,0,0,0,L,L,t5]);
      else if(arp)s.n=arp[k%arp.length];
      else{ /* motif : notes de l'accord sur les temps, notes de passage entre, reprise à la demi-mesure, fin résolue */
        if(k===last)d=near(d);else if(c>=8&&st[c-8].n>=0)d=st[c-8].n;else if(c%4===0)d=near(d+pick([-1,0,1]));
        else d=Math.max(0,Math.min(L+t5,d+pick([-2,-1,-1,1,1,2])));
        s.n=d;}
      s.a=c%4===0&&Math.random()<.4?1:0;
      /* durées : arpège régulier, basse plutôt courte, motif avec quelques notes tenues */
      s.l=Math.min(f,psy?pick([.5,.75]):arp?al:ty==='bass'?(Math.random()<.3?f:pick([.5,.75,1])):k===last?4:Math.random()<.4?pick([1,2,2,3]):pick([.5,.75,1]));
      if(f===1&&k<last&&Math.random()<(ty==='bass'?.18:.08))s.s=1;});}
  tr.steps=st;if(i===S.sel)renderRoll();save();}

// Varier une mélodie : comme « Varier » pour le rythme, 3 ou 4 retouches dans la gamme, le motif reste reconnaissable
function varyMel(i){const tr=S.tracks[i],L=SL(),mx=2*L,st=tr.steps,on=st.map((s,c)=>s.n>=0?c:-1).filter(c=>c>=0);
  if(!on.length)return randomMel(i);
  for(let k=0;k<3+rnd(2);k++){const c=pick(on),s=st[c],r=Math.random();
    if(r<.6)s.n=Math.max(0,Math.min(mx,s.n+pick([-2,-1,1,2])));
    else if(r<.8){const e=st.findIndex((x,j)=>x.n<0&&j%2===0&&j>c);if(e>=0)st[e]={n:s.n,a:0,s:0,o:0,l:1};}
    else s.a=s.a?0:1;}
  delete LP['t'+i];if(i===S.sel)renderRoll();save();}
/* ---------- Séquenceur ---------- */
// Loop : renvoie le pas à jouer pour une piste/ligne (boucle de N pas calée sur la dernière note)
function lstep(key,o,st,has){const n=o.lp||0;if(!n){delete LP[key];return st;}
  let l=LP[key];if(!l){let a=st;for(let k=0;k<16;k++){const c=(st-k+16)%16;if(has(c)){a=c;break;}}l=LP[key]={a,k:(st-a+16)%16};}
  return(l.a+(l.k++%n))%16;}
function sched(st,t){const sd=60/S.bpm/4;midiClock(t,sd);if(st%2)t+=sd*S.swing*.5;
  if(st===0){bar++;cix=(bar-1)%4;if(S.auto&&bar>1&&(bar-1)%S.auto===0&&!rec)randomRhythm();if(queued)applyR(queued);
    /* Auto-random par ligne de batterie (réglé sur chaque pad de la vue LIVE) */
    S.rows.forEach((row,r)=>{if(row.ar&&bar>1&&(bar-1)%row.ar===0&&!rec)randomRow(r);});
    /* Auto-random par piste mélodique (pas pendant un glisser ni pendant un enregistrement REC sur cette piste) */
    S.tracks.forEach((tr,i)=>{if(tr.ar&&bar>1&&(bar-1)%tr.ar===0&&!(dg&&dg.i===i)&&!rec)randomMel(i);});}
  /* Reverse batterie : la ligne est lue en miroir, chaque coup monte sur 2 pas et son attaque retombe sur la grille */
  const hs=o=>o.hf?((st>>1)+((bar&1)?0:8))%16:st; /* DEMI : la piste lue deux fois moins vite (1 pas sur 2, sur 2 mesures) */
  const gate=(b,o)=>{if(!b||!b.gt)return;const d=o.gt||0;b.gt.gain.cancelScheduledValues(t);b.gt.gain.setValueAtTime(1,t);if(d>0)b.gt.gain.setValueAtTime(Math.max(0,1-d),t+sd*.5);};
  if(drumBus)gate(drumBus,S.dfx);S.tracks.forEach((tr,i)=>V[i]&&gate(V[i].b,tr));
  const co=chOff(),g=cur().g,lr=lv('r'),lt=lv('t');for(let r=0;r<8;r++){const row=S.rows[r];if(row.hf&&st%2)continue;if(row.sk&&Math.random()<row.sk)continue;const rv=lr.rv||rvR===-1||rvR===r||!!row.rv,gr=rv?g[r].map((_,x)=>g[r][(30-x)%16]):g[r],
      c=lstep('r'+r,{lp:lr.lp||row.lp},hs(row),x=>gr[x]),n=lr.rl||row.rl||1;if(gr[c])for(let j=0;j<n;j++)drum(r,t+j*sd/n,gr[c],Math.pow(.8,j),rv?sd*2:0);}
  S.tracks.forEach((tr,i)=>{const v=V[i];if(!v)return;if(tr.hf&&st%2)return;const lp=lt.lp||tr.lp||0,q=lt.rv||i===rvI||tr.bw||tr.rv?{steps:revSteps(tr)}:tr,c=lstep('t'+i,{lp},hs(tr),x=>q.steps[x].n>=0),s=q.steps[c];
    if(tr.sk&&s.n>=0&&Math.random()<tr.sk)return;
    if(i===liveI)return; /* touche tenue : le jeu en direct a la priorité sur le motif */
    if(s.n<0){if(v.sl&&tails(q)[c]<0)off(i,t);return;}
    const to=t+(s.o||0)*sd; /* décalage de la note dans le pas (contre-temps) */
    if(tr.type==='pad'){let k=1;if(s.l&&s.l!==1)k=nlen(q,c);else while(k<16&&q.steps[(c+k)%16].n<0)k++;if(lp)k=Math.min(k,lp);note(i,s.n+co,to,sd*k*.9,s.a,0);return;}
    const n=lt.rl||tr.rl||1,L=lp?Math.min(nlen(q,c),lp):nlen(q,c);
    for(let j=0;j<n;j++)note(i,s.n+co,to+j*sd/n,L>1&&j===n-1?sd*(L-.3)-j*sd/n:sd/n*.55*Math.min(1,L),s.a&&!j,s.s&&j===n-1,Math.pow(.78,j));});
  const ap=((bar-1)%(S.am.n||4))*16+st; /* position dans la boucle des mouvements enregistrés */
  setTimeout(()=>{hl(st);if(!st)showCh();if(playing)auStep(ap);},Math.max(0,(t-ctx.currentTime)*1000));}
function tick(){while(nextT<ctx.currentTime+.1){sched(step,nextT);nextT+=60/S.bpm/4;step=(step+1)%16;}}
function toggle(){initAudio();ctx.resume();playing=!playing;$('#play').textContent=playing?'■':'▶';$('#play').classList.toggle('on',playing);
  if(playing){step=0;bar=0;cix=0;LP={};nextT=ctx.currentTime+.06;midiRun(true,nextT);timer=setInterval(tick,25);}
  else{clearInterval(timer);S.tracks.forEach((_,i)=>off(i,ctx.currentTime));midiRun(false);if(queued)applyR(queued);hl(-1);showCh();if(arec)amRec();else amUI();amRel();}}

/* ---------- Reverse : tant que le bouton (ou la touche X) est tenu, la piste sélectionnée joue à l'envers ---------- */
// Motif lu en miroir : chaque note garde sa durée, sa fin devient son début
function revSteps(tr){const st=stepsNew(),pad=tr.type==='pad';
  tr.steps.forEach((s,c)=>{if(s.n<0)return;let L=nlen(tr,c);
    if(pad&&(s.l||1)===1){L=1;while(L<16&&tr.steps[(c+L)%16].n<0)L++;}
    const p=Math.max(0,Math.min(15.75,Math.round((16-c-(s.o||0)-L)*4)/4)),k=Math.floor(p);
    st[k]={n:s.n,a:s.a,s:s.s,l:L,o:p-k};});
  return st;}
// src : 1 = bouton (souris/doigt), 2 = touche X
function revOn(src){rvS|=src;if(rvI>=0)return;rvI=S.sel;delete LP['t'+rvI];const b=$('#rev');if(b)b.classList.add('on');msg('Reverse sur '+T().name+' : relâche pour revenir à la normale');}
function revOff(src){rvS&=~src;if(rvS||rvI<0)return;delete LP['t'+rvI];rvI=-1;const b=$('#rev');if(b)b.classList.remove('on');msg('Reverse relâché : lecture normale');}
$('#fx').addEventListener('pointerdown',e=>{if(e.target.id!=='rev'||e.button>0)return;e.preventDefault();try{e.target.releasePointerCapture(e.pointerId);}catch(_){}revOn(1);});
$('#fx').addEventListener('contextmenu',e=>{if(e.target.id==='rev')e.preventDefault();});
['pointerup','pointercancel'].forEach(ev=>document.addEventListener(ev,()=>revOff(1)));
window.addEventListener('blur',()=>revOff(3));

/* ---------- Sens de lecture : bouton à bascule (ou touche V), la piste sélectionnée lit son motif de droite à gauche tant qu'il reste allumé ---------- */
function bwToggle(){const tr=T();tr.bw=tr.bw?0:1;delete LP['t'+S.sel];renderTabs();renderRoll();if(playing)hl(curStep);save();
  msg(tr.bw?'Lecture de droite à gauche sur '+tr.name+' : reclique (ou touche V) pour revenir au sens normal':'Lecture normale (gauche → droite) sur '+tr.name);}
$('#fx').addEventListener('click',e=>{if(e.target.id==='bw')bwToggle();});

/* ---------- Reverse batterie : tant que le bouton (ou la touche C) est tenu, la ligne choisie dans « Effet sur » (ou toutes) joue à l'envers ---------- */
const drvLP=()=>ROWS.forEach((_,k)=>{if(rvR===-1||rvR===k)delete LP['r'+k];});
// src : 1 = bouton (souris/doigt), 2 = touche C
function drevOn(src){rvDS|=src;if(rvR>-2)return;rvR=fxR;drvLP();const b=$('#drev');if(b)b.classList.add('on');msg('Reverse sur '+(rvR<0?'toute la batterie':ROWS[rvR])+' : relâche pour revenir à la normale');}
function drevOff(src){rvDS&=~src;if(rvDS||rvR<-1)return;drvLP();rvR=-2;const b=$('#drev');if(b)b.classList.remove('on');msg('Reverse batterie relâché : lecture normale');}
$('#dfx').addEventListener('pointerdown',e=>{if(e.target.id!=='drev'||e.button>0)return;e.preventDefault();try{e.target.releasePointerCapture(e.pointerId);}catch(_){}drevOn(1);});
$('#dfx').addEventListener('contextmenu',e=>{if(e.target.id==='drev')e.preventDefault();});
['pointerup','pointercancel'].forEach(ev=>document.addEventListener(ev,()=>drevOff(1)));
window.addEventListener('blur',()=>drevOff(3));

/* ---------- Jeu au clavier ---------- */
function liveOn(d){if(!ctx)return;ctx.resume();const i=S.sel;if(liveI>=0&&liveI!==i)off(liveI,ctx.currentTime);liveI=i;if(V[i])V[i].sl=0;note(i,d,ctx.currentTime,0,0,0);
  if(rec&&playing&&curStep>=0){const st=recStep(),n=Math.max(0,Math.min(2*SL(),d-chOff()));T().steps[st]={n,a:0,s:0,o:0,l:1};recH={i,st,t:ctx.currentTime};
    renderRoll();save();if(window.recFlash)recFlash(i,st,n);}}
/* REC : la note va sur le pas le plus proche (jouée un peu en retard = le pas en cours, au-delà de la moitié = le suivant) */
let recH=null;
function recStep(){const sd=60/S.bpm/4,c=(ctx.currentTime-stT)/sd>.5?(curStep+1)%16:curStep;return T().bw?15-c:c;}
function liveOff(){if(!ctx||liveI<0)return;
  if(recH&&recH.i===liveI){const sd=60/S.bpm/4,l=Math.max(1,Math.min(16,Math.round((ctx.currentTime-recH.t)/sd))),s=S.tracks[recH.i].steps[recH.st];if(s&&s.n>=0){s.l=l;for(let k=1;k<l&&k<16;k++){const x=S.tracks[recH.i].steps[(recH.st+k)%16];if(x)x.n=-1;}renderRoll();save();}recH=null;}
  off(liveI,ctx.currentTime);liveI=-1;}

/* ---------- Notes longues : on tire une note vers la droite dans la grille ---------- */
// Longueur réelle d'une note en pas (elle s'arrête à la note suivante)
function nlen(tr,c){const s=tr.steps[c],o=s.o||0,l=s.l||1;let k=1;while(k<o+l&&tr.steps[(c+k)%16].n<0)k++;
  return k<o+l?Math.min(l,k+(tr.steps[(c+k)%16].o||0)-o):l;}
// Pour chaque pas : degré de la note tenue qui le recouvre (-1 = aucun)
function tails(tr){const h=Array(16).fill(-1);h.w=Array(16).fill(1);tr.steps.forEach((s,c)=>{if(s.n<0)return;const e=(s.o||0)+nlen(tr,c);for(let j=1;j<e&&c+j<16;j++){h[c+j]=s.n;h.w[c+j]=Math.min(1,e-j);}});return h;}
// Part remplie d'une case : une note peut s'arrêter au quart de pas
const nsty=(tr,tl,c,d)=>{const s=tr.steps[c],o=s.o||0;
  return s.n===d?`--x:${o*100}%;--w:${Math.min(1,o+nlen(tr,c))*100}%;--t:${tl[c]===d?tl.w[c]*100:0}%`:tl[c]===d?`--w:${tl.w[c]*100}%`:'';};
function ncls(tr,tl,c,d){const s=tr.steps[c];return(s.n===d?'n'+(s.a?' ac':'')+(s.s?' sl':''):tl[c]===d?'h':'')+(c%4?'':' b');}
// Met à jour les couleurs sans reconstruire la grille (indispensable pendant un glisser)
function paintRoll(){const tr=T(),tl=tails(tr);document.querySelectorAll('#roll i[data-d]').forEach(e=>{const c=+e.dataset.c;e.className=ncls(tr,tl,c,+e.dataset.d)+(c===rst()?' pl':'');e.style.cssText=nsty(tr,tl,c,+e.dataset.d);});}
let dg=null,tool=0;
$('#roll').addEventListener('pointerdown',e=>{const d=e.target.dataset;if(d.d==null||e.button>0)return;
  const tr=T(),c=+d.c,dd=+d.d,head=tr.steps[c].n===dd;let c0=c;
  if(!head&&tails(tr)[c]===dd){c0=c-1;while(c0>0&&tr.steps[c0].n<0)c0--;}
  const bx=e.target.getBoundingClientRect(),m=head?0:c0!==c?1:2;
  /* mo = outil Déplacer (ou Maj) · gp = point saisi · p0 = début de la note · ln = sa durée réelle */
  dg={i:S.sel,c,c0,d:dd,m,mv:false,x:e.clientX,snap:JSON.stringify(tr.steps),
    mo:!!(tool||e.shiftKey),gp:c+(e.clientX-bx.left)/bx.width,p0:m<2?c0+(tr.steps[c0].o||0):c,ln:m<2?nlen(tr,c0):1};});
document.addEventListener('pointermove',e=>{if(!dg)return;
  if(e.pointerType==='mouse'&&!e.buttons)return dragEnd(e);
  const el=document.elementFromPoint(e.clientX,e.clientY),ce=el&&el.closest&&el.closest('#roll i[data-c]');if(!ce)return;
  const cc=+ce.dataset.c,bx=ce.getBoundingClientRect(),fr=(e.clientX-bx.left)/bx.width,q=Math.min(4,Math.max(1,Math.ceil(fr*4)))/4;if(!dg.mv&&Math.abs(e.clientX-dg.x)<5)return;dg.mv=true;
  const st=JSON.parse(dg.snap),E=()=>({n:-1,a:0,s:0});let h,c0=dg.c0;
  if(dg.mo){ /* Déplacer : la note glisse par ¼ de pas en gardant sa durée */
    const src=dg.m<2?st[dg.c0]:E();if(dg.m<2)st[dg.c0]=E();
    const p=Math.max(0,Math.min(16-dg.ln,dg.p0+Math.round((cc+fr-dg.gp)*4)/4));c0=dg.nc=Math.floor(p);
    h=st[c0]={n:dg.d,a:src.a,s:src.s,l:dg.ln,o:p-c0};}
  else{h=st[c0];h.n=dg.d;if(dg.m===2)h.o=0;h.l=Math.max(.25,cc+q-c0-(h.o||0));}
  const en=c0+(h.o||0)+h.l;
  for(let k=c0+1;k<16&&k<en;k++)if(st[k].n>=0&&k+(st[k].o||0)<en)st[k]=E();
  S.tracks[dg.i].steps=st;paintRoll();});
function dragEnd(e){if(!dg)return;const g=dg,tr=S.tracks[g.i];dg=null;
  if(e.type==='pointercancel'){if(g.mv){tr.steps=JSON.parse(g.snap);renderRoll();}return;}
  const s=tr.steps[g.mo&&g.mv?g.nc:g.c0],o=s.o||0;
  if(g.mv)msg(g.mo?'Note déplacée : pas '+(g.nc+1)+(o?' + '+['','¼','½','¾'][o*4]+' de pas':''):s.l!==1?'Durée de la note : '+String(s.l).replace('.',',')+' pas':'Note d’un pas');
  else if(g.m===0){s.n=-1;s.l=1;s.o=0;}
  else if(g.m===1){s.l=Math.max(.25,g.c-g.c0-o);}
  else{s.n=g.d;s.l=1;s.o=0;if(!playing&&ctx){if(V[g.i])V[g.i].sl=0;note(g.i,s.n,ctx.currentTime,.2,0,0);}}
  renderRoll();save();}
['pointerup','pointercancel'].forEach(ev=>document.addEventListener(ev,dragEnd));
// Outil de la grille : Durée (tirer le bout de la note) ou Déplacer (glisser la note, contre-temps)
$('#tool').onclick=e=>{const v=e.target.dataset.tl;if(v==null)return;tool=+v;
  $('#tool').querySelectorAll('button').forEach(b=>b.classList.toggle('on',b.dataset.tl===v));$('#roll').classList.toggle('mvt',!!tool);
  msg(tool?'Outil Déplacer : glisse une note pour la décaler par ¼ de pas (contre‑temps)':'Outil Durée : tire une note pour l’allonger ou la raccourcir');};

/* ---------- Import de sons ---------- */
async function addFiles(list,trk,folder){if(!ctx)return msg('Audio indisponible dans ce navigateur');let last='';
  for(const f of list){try{const ab=f.dataUrl?await(await fetch(f.dataUrl)).arrayBuffer():await f.arrayBuffer();
      const id='s'+Date.now().toString(36)+rnd(9999);
      bufs[id]=await ctx.decodeAudioData(ab.slice(0));const nm=f.name.replace(/\.\w+$/,''),pth=f.webkitRelativePath||f.path||'';let fd=folder||guessF(nm,pth);
      if(!fd&&pth.includes('/')){const par=pth.split('/').slice(-2)[0].slice(0,24);if(par){if(!libFolders().includes(par))(S.libF=S.libF||[]).push(par);fd=par;}}
      S.lib.push({id,name:nm,f:fd||(trk===true?TYPF[T().type]:'Samples')});
      const gz=trk!==true&&GUESS.find(([r,re])=>re.test(f.name)&&!S.rows[r].smp);if(gz)S.rows[gz[0]].smp=id;last=id;
      idb('readwrite',s=>s.put(ab,id)).catch(()=>{});
    }catch(e){msg('Fichier audio illisible : '+f.name);}}
  renderRhythm();renderSnd();renderLib();save();
  if(trk===true&&last)setSmp(last);else msg(list.length+' son(s) ajouté(s) — '+S.lib.length+' dans la bibliothèque, rangés par dossier');}

/* ---------- Événements ---------- */
document.addEventListener('pointerdown',()=>{if(ctx)ctx.resume();});
document.addEventListener('click',e=>{const el=e.target.closest('[data-mr],[data-mt],[data-st],[data-rk],[data-lp],[data-rl],[data-so],[data-gm],[data-lt],[data-rm],i[data-c]');if(!el)return;const d=el.dataset;
  if(d.so!=null)return solo(d.so);
  if(d.rm!=null){randomMel(+d.rm);delete LP['t'+d.rm];return msg('Nouvelle mélodie sur '+S.tracks[d.rm].name+(playing?'':' — appuie sur ▶ pour l’écouter'));}
  if(d.gm!=null){S.gm[d.gm]=S.gm[d.gm]?0:1;applyParams();renderPads();}
  else if(d.lt!=null){S.lt=d.lt;LP={};renderPads();}
  else if(d.mr!=null){S.rows[d.mr].mute=!S.rows[d.mr].mute;renderRhythm();}
  else if(d.mt!=null){S.tracks[d.mt].mute=!S.tracks[d.mt].mute;applyParams();renderPads();renderTabs();}
  else if(d.st!=null){S.sel=+d.st;renderTabs();renderRoll();}
  else if(d.rk!=null){go(S.rhythms[d.rk]);return;}
  else if(d.lp!=null||d.rl!=null){const dr=el.closest('#dfx'),k=d.lp!=null?'lp':'rl',v=+(d.lp!=null?d.lp:d.rl);
    if(dr){(fxR<0?S.rows:[S.rows[fxR]]).forEach(o=>o[k]=v);renderRhythm();}
    else{T()[k]=v;renderRoll();}}
  else if(d.r!=null){const g=cur().g;g[d.r][d.c]=g[d.r][d.c]?0:1;cur().tmp=false;renderRhythm();}
  else if(d.d!=null)return; /* notes de la grille : gérées par le glisser (section « Notes longues ») */
  else if(d.m){const s=T().steps[d.c];s[d.m]=s[d.m]?0:1;renderRoll();}
  save();});
$('#drums').addEventListener('contextmenu',e=>{const d=e.target.dataset;if(d.r==null)return;e.preventDefault();const g=cur().g;g[d.r][d.c]=g[d.r][d.c]===2?1:2;cur().tmp=false;renderRhythm();save();});
['input','change'].forEach(ev=>{
  $('#drums').addEventListener(ev,e=>{const d=e.target.dataset;if(d.sr!=null)S.rows[d.sr].smp=e.target.value;if(d.vr!=null){S.rows[d.vr].vol=+e.target.value;pvSync('r'+d.vr,S.rows[d.vr].vol);applyParams();}save();});
  $('#fx').addEventListener(ev,e=>{const p=e.target.dataset.p;if(!p)return;T()[p]=p==='wave'||p==='ftype'?e.target.value:+e.target.value;if(p==='po')$('#pv').textContent=pv(T().po);if(p==='oct')T().oct=Math.min(6,Math.max(1,Math.round(T().oct)||2));if(p==='vol')pvSync('t'+S.sel,T().vol);applyParams();save();});
});
$('#play').onclick=toggle;
/* ---------- LIVE : solo, effets à maintenir (Loop, Roll, Reverse), filtres ---------- */
function solo(k){const j=S.solo.indexOf(k);if(j<0)S.solo.push(k);else S.solo.splice(j,1);applyParams();renderPads();save();}
function lvSet(k,v){LV[k]=v;LP={};document.querySelectorAll('[data-hl]').forEach(b=>b.classList.toggle('on',LV[b.dataset.hl]===+b.dataset.v));}
const lvUp=()=>{if(lvH){lvSet(lvH,0);lvH=null;}};
$('#lfx').addEventListener('pointerdown',e=>{const d=e.target.dataset;if(!d.hl||e.button>0)return;e.preventDefault();try{e.target.releasePointerCapture(e.pointerId);}catch(_){}
  const v=+d.v;amT(d.hl);if(latch)lvSet(d.hl,LV[d.hl]===v?0:v);else{lvUp();lvH=d.hl;lvSet(lvH,v);}});
$('#lfx').addEventListener('contextmenu',e=>{if(e.target.dataset.hl)e.preventDefault();});
['pointerup','pointercancel'].forEach(ev=>document.addEventListener(ev,lvUp));
window.addEventListener('blur',lvUp);
$('#lat').onclick=()=>{latch=!latch;$('#lat').classList.toggle('on',latch);if(!latch)['lp','rl','rv'].forEach(k=>lvSet(k,0));
  msg(latch?'Verrou : un clic garde l’effet, un second clic le coupe':'Verrou coupé : les effets tiennent tant que tu appuies');};
$('#soOff').onclick=$('#soOff2').onclick=()=>{S.solo=[];applyParams();renderPads();save();};
// Auto-random réglé sur chaque pad de la vue LIVE (ligne de batterie ou piste mélodique)
document.addEventListener('change',e=>{const k=e.target.dataset&&e.target.dataset.au;if(k==null)return;
  const v=+e.target.value,i=+k.slice(1),dr=k[0]==='r',nm=dr?ROWS[i]:S.tracks[i].name;
  if(dr)S.rows[i].ar=v;else{S.tracks[i].ar=v;renderTabs();if(i===S.sel)$('#mauto').value=v;}
  document.querySelectorAll('[data-au="'+k+'"]').forEach(x=>{x.value=v;x.classList.toggle('on',!!v);});e.target.blur();save();
  msg(v?'Auto '+nm+' : '+(dr?'nouveau motif':'nouvelle mélodie')+' toutes les '+v+' mesures':'Auto coupé sur '+nm);});
// Volume de chaque piste depuis la vue LIVE (jauge sous le pad), synchronisé avec les curseurs des sections RYTHME et MÉLODIE
const pvSync=(k,v)=>document.querySelectorAll('[data-pv="'+k+'"]').forEach(e=>{e.value=v;});
// Pendant qu'une jauge est tenue, les pads ne sont pas reconstruits (sinon le glisser se couperait)
const pvEnd=()=>{pvD=0;if(pvQ){pvQ=0;setTimeout(renderPads,0);}};
document.addEventListener('pointerdown',e=>{if(e.target.dataset&&e.target.dataset.pv!=null)pvD=e.pointerId+1;});
['pointerup','pointercancel'].forEach(ev=>document.addEventListener(ev,e=>{if(pvD&&e.pointerId+1===pvD)pvEnd();}));
window.addEventListener('blur',()=>{if(pvD)pvEnd();});
document.addEventListener('input',e=>{const k=e.target.dataset&&e.target.dataset.pv;if(k==null)return;
  const v=+e.target.value,i=+k.slice(1),dr=k[0]==='r';let o=null;
  if(dr){S.rows[i].vol=v;o=document.querySelector('[data-vr="'+i+'"]');}
  else{S.tracks[i].vol=v;if(i===S.sel)o=document.querySelector('#fx [data-p="vol"]');}
  if(o)o.value=v;document.querySelectorAll('[data-pv="'+k+'"]').forEach(x=>{if(x!==e.target)x.value=v;});applyParams();save();
  msg('Volume '+(dr?ROWS[i]:S.tracks[i].name)+' : '+Math.round(v*100)+' %');});
/* ---------- LIVE : filtres & effets généraux + enregistrement de leurs mouvements (boucle de N mesures) ---------- */
const AK=['mf','hp','mq','xd','xr','xs'];
let arec=false,aT={},aH='';
// Marque un réglage comme « bougé » pendant le REC : lui seul est réécrit dans la boucle
const amT=k=>{if(!arec||aT[k])return;aT[k]=1;if(!S.am.d[k])S.am.d[k]=Array((S.am.n||4)*16).fill(null);amUI();};
// Relâche les effets Loop / Roll / Reverse laissés actifs par la boucle enregistrée
const amRel=()=>['lp','rl','rv'].forEach(k=>{if(S.am.d[k]&&LV[k]&&lvH!==k)lvSet(k,0);});
function amUI(){const d=S.am.d,has=Object.keys(d).length;
  $('#amRec').classList.toggle('on',arec);$('#amRec').textContent=arec?'■ Stop':'⏺ REC';
  $('#amOn').classList.toggle('on',!!S.am.on&&has>0);$('#amOn').textContent=S.am.on?'▶ Rejoue':'⏸ Pause';$('#amOn').disabled=!has;$('#amClr').disabled=!has;
  $('#amN').value=S.am.n||4;
  document.querySelectorAll('[data-am]').forEach(e=>e.closest('label').classList.toggle('aut',!!d[e.dataset.am]));
  document.querySelectorAll('[data-hl]').forEach(b=>b.closest('.lbl').classList.toggle('aut',!!d[b.dataset.hl]));
  if(!playing)$('#amP').textContent=has?has+' réglage(s) enregistré(s)':'Aucun mouvement enregistré';}
// Appelé à chaque pas, au moment où il sonne : écrit les réglages bougés (REC) puis rejoue les autres
function auStep(p){const d=S.am.d;let ch=0;
  if(arec)for(const k in aT)(d[k]||(d[k]=[]))[p]=k in LV?LV[k]:k[0]==='k'&&window.amGetK?amGetK(k):S[k];
  if(S.am.on)for(const k in d){const v=d[k]&&d[k][p];if(v==null||(arec&&aT[k])||k===aH)continue;
    if(k in LV){if(lvH!==k&&LV[k]!==v)lvSet(k,v);}
    else if(AK.includes(k)&&S[k]!==v){S[k]=v;$('#'+k).value=v;ch=1;}
    else if(k[0]==='k'&&window.amSetK)amSetK(k,v);}
  if(ch)applyParams();
  if(p%16===0)$('#amP').textContent=(arec?'⏺ REC · ':'')+'mesure '+(p/16+1)+' / '+(S.am.n||4);}
function amRec(){arec=!arec;aT={};if(arec){S.am.on=1;if(!playing)toggle();}else save();amUI();
  msg(arec?'REC effets : bouge les filtres, les effets ou maintiens Loop / Roll / Reverse — boucle de '+(S.am.n||4)+' mesure(s)':'REC effets coupé : tes mouvements rejouent en boucle');}
$('#amRec').onclick=amRec;
$('#amOn').onclick=()=>{S.am.on=S.am.on?0:1;if(!S.am.on)amRel();amUI();save();msg(S.am.on?'Mouvements enregistrés : lecture en boucle':'Mouvements enregistrés en pause : tu as la main sur les réglages');};
$('#amClr').onclick=()=>{amRel();S.am.d={};aT={};amUI();save();msg('Mouvements enregistrés effacés');};
$('#amN').onchange=e=>{S.am.n=+e.target.value||4;e.target.blur();amUI();save();msg('Boucle des mouvements : '+S.am.n+' mesure(s)');};
document.addEventListener('input',e=>{const k=e.target.dataset&&e.target.dataset.am;if(k)amT(k);});
// Tant qu'un curseur du panneau est tenu, la boucle enregistrée ne le bouge pas
document.addEventListener('pointerdown',e=>{const k=e.target.dataset&&e.target.dataset.am;if(k)aH=k;});
['pointerup','pointercancel'].forEach(ev=>document.addEventListener(ev,()=>{aH='';}));
window.addEventListener('blur',()=>{aH='';});
$('#fl0').onclick=()=>{Object.assign(S,{mf:1,hp:0,mq:0,xd:0,xr:0,xs:0});AK.forEach(k=>{$('#'+k).value=S[k];amT(k);});applyParams();save();
  msg(!arec&&S.am.on&&AK.some(k=>S.am.d[k])?'Tout à zéro — les mouvements enregistrés reprennent la main : « Effacer » ou « Pause » pour les arrêter':'Filtres et effets généraux à zéro');};
$('#dfx').addEventListener('change',e=>{if(e.target.id==='fxRow'){fxR=+e.target.value;renderDfx();}});
$('#fx').addEventListener('dblclick',e=>{if(e.target.dataset.p!=='po')return;T().po=0;applyParams();renderRoll();save();});
$('#rnd').onclick=()=>randomRhythm();
// Vue LIVE : style + rythme random pour toute la batterie
$('#lstyle').onchange=e=>{setStyle(e.target.value);styleSc();e.target.blur();save();};
$('#lrnd').onclick=()=>{randomRhythm();msg('Nouveau rythme '+S.style[0].toUpperCase()+S.style.slice(1)+(playing?' : il démarre à la prochaine mesure':' — appuie sur ▶ pour l’écouter'));};
$('#vary').onclick=()=>{const g=cur().g;for(let k=0;k<4;k++){const r=3+rnd(5),c=rnd(16);g[r][c]=g[r][c]?0:1;}cur().tmp=false;renderRhythm();save();};
$('#keep').onclick=()=>{cur().tmp=false;renderBank();save();msg('Matrice gardée dans la banque');};
$('#dup').onclick=()=>{const c=JSON.parse(JSON.stringify(cur()));c.name=(c.name+' bis').slice(0,24);c.tmp=false;S.rhythms.splice(S.cur+1,0,c);go(c);};
$('#clr').onclick=()=>{cur().g=ROWS.map(()=>Array(16).fill(0));cur().tmp=false;renderRhythm();save();};
$('#del').onclick=()=>{if(S.rhythms.length<2)return msg('Il faut garder au moins une matrice');S.rhythms.splice(S.cur,1);S.cur=Math.max(0,S.cur-1);queued=null;renderRhythm();save();};
$('#rname').oninput=e=>{cur().name=e.target.value;cur().tmp=false;renderBank();save();};
$('#mrnd').onclick=()=>randomMel(S.sel);
$('#mvary').onclick=()=>{varyMel(S.sel);msg('Mélodie de '+T().name+' variée');};
$('#mall').onclick=()=>{let n=0;S.tracks.forEach((t,i)=>{if(!t.mute){randomMel(i);n++;}});msg(n?n+' piste(s) régénérée(s) sur les mêmes accords':'Toutes les pistes sont coupées : rien à générer');};
$('#mauto').onchange=e=>{const v=+e.target.value;T().ar=v;renderTabs();renderPads();save();msg(v?'Auto‑random '+T().name+' : nouvelle mélodie toutes les '+v+' mesures':'Auto‑random coupé sur '+T().name);};
$('#mclr').onclick=()=>{T().steps=stepsNew();renderRoll();save();};
$('#msave').onclick=()=>{const tr=T();if(!tr.steps.some(s=>s.n>=0))return msg('Piste vide : rien à enregistrer');
  if(S.mels.length>=40)return msg('Banque pleine (40 mélodies) : supprime-en une avant d’enregistrer');
  const nm=($('#mname').value.trim()||tr.name+' '+(S.mels.length+1)).slice(0,24);
  S.mels.push({name:nm,t:S.sel,steps:JSON.parse(JSON.stringify(tr.steps))});$('#mname').value='';mdel=-1;renderMels();save();msg('Mélodie « '+nm+' » enregistrée');};
$('#mbank').addEventListener('click',e=>{const d=e.target.dataset;
  if(d.ml!=null){const m=S.mels[d.ml];if(!m)return;const mx=2*SL();
    T().steps=stepsNew().map((s,c)=>{const x=(m.steps||[])[c]||s,o=Math.min(.75,Math.max(0,Math.round((+x.o||0)*4)/4));return{n:Math.min(mx,x.n),a:x.a?1:0,s:x.s?1:0,o,l:Math.min(16-c-o,Math.max(.25,Math.round((+x.l||1)*4)/4))};});
    delete LP['t'+S.sel];mdel=-1;renderRoll();renderMels();save();msg('« '+m.name+' » chargée sur '+T().name+(playing?'':' — appuie sur ▶ pour l’écouter'));}
  else if(d.mx!=null){if(mdel===+d.mx){S.mels.splice(mdel,1);mdel=-1;save();msg('Mélodie supprimée');}else mdel=+d.mx;renderMels();}});
$('#rec').onclick=()=>{rec=!rec;$('#rec').classList.toggle('on',rec);msg(rec?'REC : lance ▶ puis joue, les notes s’écrivent dans la piste':'REC coupé');};
$('#imp').onclick=()=>{impT=false;$('#file').click();};
$('#timp').onclick=()=>{impT=true;$('#file').click();};
$('#file').onchange=e=>{addFiles([...e.target.files],impT);e.target.value='';};
$('#timpD').onclick=async()=>{if(!window.deskosia)return msg('Disponible uniquement dans Deskosia');try{addFiles(await deskosia.files.pick({multiple:true,accept:'audio/*'}),true);}catch(e){msg('Sélection impossible : '+e.message);}};
$('#tsmp').onchange=e=>setSmp(e.target.value);
$('#tsn').onchange=e=>{T().sn=+e.target.value;save();};
$('#tss').oninput=e=>{T().ss=+e.target.value;save();};
$('#impD').onclick=async()=>{if(!window.deskosia)return msg('Disponible uniquement dans Deskosia');try{addFiles(await deskosia.files.pick({multiple:true,accept:'audio/*'}));}catch(e){msg('Sélection impossible : '+e.message);}};
$('#keys').addEventListener('pointerdown',e=>{if(e.target.dataset.k!=null)liveOn(+e.target.dataset.k);});
['pointerup','pointerleave','pointercancel'].forEach(ev=>$('#keys').addEventListener(ev,liveOff));
window.addEventListener('blur',liveOff);
document.addEventListener('keydown',e=>{if(window.VIEW==='fm1')return;const t=e.target;if(/INPUT|SELECT|TEXTAREA/.test(t.tagName)&&t.type!=='range')return;if(e.repeat)return;
  if(e.code==='Space'){e.preventDefault();toggle();return;}
  if(e.code==='KeyX'){revOn(2);return;}
  if(e.code==='KeyC'&&!e.ctrlKey&&!e.metaKey){drevOn(2);return;}
  if(e.code==='KeyV'&&!e.ctrlKey&&!e.metaKey){bwToggle();return;}
  const k=KEYS.indexOf(e.code);if(k>=0&&k<=2*SL()){liveOn(k);return;}
  const m=/^Digit([1-8])$/.exec(e.code);if(m){if(e.shiftKey)return solo('r'+(m[1]-1));const r=S.rows[m[1]-1];r.mute=!r.mute;renderRhythm();save();}});
document.addEventListener('keyup',e=>{if(window.VIEW==='fm1')return;if(e.code==='Space'&&!/INPUT|TEXTAREA/.test(e.target.tagName))e.preventDefault();if(KEYS.includes(e.code))liveOff();if(e.code==='KeyX')revOff(2);if(e.code==='KeyC')drevOff(2);});
$('#midi').onclick=()=>midiConnect();
$('#aiGo').onclick=async()=>{const q=$('#aiQ').value.trim();if(!q)return;if(!window.deskosia)return msg('IA disponible uniquement dans Deskosia');
  const L=SL();msg('IA en cours…');
  try{const r=await deskosia.ai({json:true,system:'Tu programmes une groovebox. Réponds uniquement en JSON valide.',
      prompt:`Demande : "${q}". Style ${S.style}, ${S.bpm} BPM. Renvoie {"name":"nom court","drums":[8 chaînes de 16 caractères "x" ou "." dans l'ordre ${ROWS.join(', ')}] ou null,"melody":[16 entiers : degré de gamme de 0 à ${2*L}, -1 = silence] ou null,"accent":[16 fois 0 ou 1],"slide":[16 fois 0 ou 1]}. Remplis "drums" si la demande parle de rythme, "melody" si elle parle de la piste ${T().name} (type ${T().type}).`});
    const j=r.json||JSON.parse(r.text);
    if(Array.isArray(j.drums)&&j.drums.length===8)go({name:('IA · '+(j.name||'rythme')).slice(0,24),tmp:false,st:S.style,g:j.drums.map(s=>Array.from({length:16},(_,c)=>String(s)[c]==='x'?1:0))},true);
    if(Array.isArray(j.melody)){T().steps=stepsNew().map((_,c)=>({n:Math.max(-1,Math.min(2*L,Math.round(Number(j.melody[c]??-1))||0)),a:j.accent&&j.accent[c]?1:0,s:j.slide&&j.slide[c]?1:0}));renderRoll();}
    save();msg('IA : appliqué ✔');
  }catch(e){msg('IA : échec ('+e.message+')');}};

/* ---------- Démarrage ---------- */
(async function(){
  let d=null;try{d=window.deskosia?await deskosia.store.get('projet'):JSON.parse(localStorage.getItem('fm1groove'));}catch(e){}
  const ok=d&&Array.isArray(d.tracks)&&Array.isArray(d.rhythms)&&d.rhythms.length;
  S=Object.assign(defState(),ok?d:{});S.cur=Math.min(S.cur,S.rhythms.length-1);
  /* Ancien projet : ajoute la piste SYNTHÉ 2 après BASSE 2 (la piste sélectionnée et les solos restent sur leur piste) */
  if(!S.tracks.some(t=>t.name==='SYNTHÉ 2')){const b=S.tracks.findIndex(t=>t.name==='BASSE 2'),at=b<0?S.tracks.length:b+1;
    S.tracks.splice(at,0,SYN2());if(S.sel>=at)S.sel++;
    if(Array.isArray(S.solo))S.solo=S.solo.map(k=>k[0]==='t'&&+k.slice(1)>=at?'t'+(+k.slice(1)+1):k);}
  $('#root').innerHTML=NOTES.map((n,i)=>`<option value="${i}">${n}</option>`).join('');
  $('#scale').innerHTML=Object.keys(SCALES).map(n=>`<option>${n}</option>`).join('');
  $('#style').innerHTML=Object.keys(STYLES).map(n=>`<option value="${n}">${n[0].toUpperCase()+n.slice(1)}</option>`).join('');
  try{initAudio();}catch(e){msg('Audio indisponible dans ce navigateur');}
  const bind=(id,k,fn)=>{const el=$(id);el.value=S[k];el.oninput=()=>{S[k]=(k==='scale'||k==='style')?el.value:+el.value;if(fn)fn();applyParams();save();};};
  bind('#bpm','bpm',()=>{S.bpm=Math.min(220,Math.max(60,S.bpm||120));});
  bind('#swing','swing');bind('#mf','mf');bind('#hp','hp');bind('#mq','mq');bind('#xd','xd');bind('#xr','xr');bind('#xs','xs');bind('#auto','auto');
  bind('#root','root',()=>{renderRoll();renderKeys();renderProg();});
  /* Changement de gamme : chaque note posée va sur la note la plus proche de la nouvelle gamme */
  let pSc=S.scale;
  bind('#scale','scale',()=>{const a=SCALES[pSc]||SCALES[S.scale],b=SCALES[S.scale],La=a.length,Lb=b.length;pSc=S.scale;
    S.tracks.forEach(t=>t.steps.forEach(s=>{if(s.n<0)return;const sem=a[s.n%La]+12*Math.floor(s.n/La);let best=0,bd=99;
      for(let d=0;d<=2*Lb;d++){const x=Math.abs(b[d%Lb]+12*Math.floor(d/Lb)-sem);if(x<bd){bd=x;best=d;}}s.n=best;}));
    renderRoll();renderKeys();renderProg();msg('Gamme '+S.scale+' : notes recalées, le générateur suit cette gamme');});
  bind('#style','style',()=>{setStyle(S.style);styleSc();});$('#style').onchange=$('#style').oninput;
  $('#lstyle').innerHTML=$('#style').innerHTML;$('#lstyle').value=S.style;
  renderProg();bind('#prog','prog',()=>{showCh();msg(prg()?'Accords : '+prg().map(chName).join(' – ')+' (un par mesure)':'Accords coupés : tout reste sur la tonique');});$('#prog').onchange=$('#prog').oninput;
  document.querySelectorAll('[data-dp]').forEach(el=>{el.value=S.dfx[el.dataset.dp];el.oninput=()=>{S.dfx[el.dataset.dp]=+el.value;applyParams();save();};});
  if(!ok)randomMel(0);
  if(!Array.isArray(S.mels))S.mels=[];
  if(!S.am||!S.am.d||typeof S.am.d!=='object')S.am={n:4,on:1,d:{}};
  /* Bibliothèque : chaque son est rangé dans un dossier */
  if(!Array.isArray(S.libF))S.libF=[];S.lib.forEach(l=>{if(!l.f)l.f=guessF(l.name)||'Samples';});
  renderRhythm();renderTabs();renderRoll();renderKeys();renderMels();amUI();libInit();midiInit();
  window.dispatchEvent(new Event('groove-ready'));
  if(window.deskosia&&deskosia.files&&deskosia.files.onFile)deskosia.files.onFile(addFiles);
  if(ctx){let miss=0;for(const l of S.lib){try{const ab=await idb('readonly',s=>s.get(l.id));if(ab)bufs[l.id]=await ctx.decodeAudioData(ab.slice(0));else miss++;}catch(e){miss++;}}
    if(miss)msg(miss+' son(s) absents de ce navigateur : réimporte-les (son interne utilisé en attendant)');renderLib();}
})();
