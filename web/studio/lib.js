/* ---------- Bibliothèque de sons : dossiers pré-écrits + dossiers à toi ---------- */
const LIBF=['Kick','Snare','Clap','Hi-hat','Open hat','Perc','Tom','Basse','Synth','Nappe','FX','Voix','Boucles','Samples'];
// Devine le dossier d'un son d'après son nom (ou le dossier d'où il vient)
// mot entier : les séparateurs _ - . et les chiffres comptent comme des espaces (kick_01, tom-low, pad.air)
const W=x=>new RegExp('(?<![a-z])(?:'+x+')(?![a-z])','i');
const LIBRE=[['Kick',/kick|bassdrum|bass ?drum/i],['Kick',W('bd|kck')],['Snare',/snare/i],['Snare',W('sd|snr')],['Clap',/clap/i],['Clap',W('cp')],['Open hat',/open ?hat|openhat/i],['Open hat',W('oh|ohh|open')],
  ['Hi-hat',/hi-?hat|hihat|closed/i],['Hi-hat',W('hh|chh|ch|hat')],['Tom',W('toms?|floor ?tom')],['Perc',/perc|conga|bongo|shaker|cowbell|tamb|rim|clave|block|maraca/i],
  ['Basse',/bass|808|reese|basse/i],['Basse',W('sub')],['Synth',/synth|lead|pluck|arp|stab|chord|keys?/i],['Nappe',/nappe|string|atmo|drone/i],['Nappe',W('pads?')],
  ['FX',/riser|sweep|impact|noise|uplift|downlift|whoosh/i],['FX',W('fx|sfx')],['Voix',/vox|voc|voice|voix|chant|acapella/i],['Boucles',/loop|boucle|break/i]];
// Dossier conseillé pour chaque ligne de batterie et chaque type de piste
const ROWF=['Kick','Snare','Clap','Hi-hat','Open hat','Perc','Tom','Perc'],TYPF={bass:'Basse',synth:'Synth',pad:'Nappe',fx:'FX'};
let libCur='',libDel='',prevSrc=null;
// Un emoji par catégorie d'instrument (dossiers, sons, lignes de batterie, pistes)
// Icônes et couleurs par catégorie : icons.js (ICONS, iconSVG, drawIcon, catC)
const catOfId=id=>id==='piano'?'Piano':(S.lib.find(l=>l.id===id)||{}).f||'Samples';
const optC=f=>` style="color:${catC(f)}"`;
const libFolders=()=>[...LIBF,...(S.libF||[]).filter(f=>!LIBF.includes(f))];
function guessF(name,path){const all=libFolders(),seg=String(path||'').split('/').slice(0,-1).reverse();
  for(const s of seg){const ex=all.find(f=>f.toLowerCase()===s.toLowerCase());if(ex)return ex;const g=LIBRE.find(([,re])=>re.test(s));if(g)return g[0];}
  const g=LIBRE.find(([,re])=>re.test(name));return g?g[0]:'';}
// Menu des sons groupé par dossier (le dossier conseillé en premier)
function smpOpts(sel,first,pref){const fs=libFolders().filter(f=>S.lib.some(l=>l.f===f));if(pref&&fs.includes(pref))fs.splice(fs.indexOf(pref),1),fs.unshift(pref);
  return `<option value="">${first}</option>`+(first==='Synthé interne'?`<option value="piano"${optC('Piano')}${sel==='piano'?' selected':''}>■ Piano (interne)</option>`:'')+fs.map(f=>`<optgroup label="■ ${esc(f)}"${optC(f)}>${S.lib.filter(l=>l.f===f).map(l=>`<option value="${l.id}"${optC(f)}${l.id===sel?' selected':''}>${esc(l.name)}</option>`).join('')}</optgroup>`).join('');}
// Pré-écoute d'un son de la bibliothèque
function preview(id){if(!ctx)return;ctx.resume();if(prevSrc)try{prevSrc.stop();}catch(e){}if(id==='piano')id=pno(60);const b=bufs[id];if(!b)return msg('Son absent de ce navigateur : réimporte-le');
  const s=ctx.createBufferSource(),g=ctx.createGain();g.gain.value=.8;s.buffer=b;s.connect(g);g.connect(master);s.start();prevSrc=s;}
// Met un son sur une ligne de batterie (r0..r7) ou une piste mélodique (t0..t5)
function libAssign(id,k){const i=+k.slice(1),l=id==='piano'?{name:'Piano'}:S.lib.find(x=>x.id===id);if(!l||id==='piano'&&k[0]==='r')return;
  if(k[0]==='r'){S.rows[i].smp=id;renderRhythm();save();msg('« '+l.name+' » sur la ligne '+ROWS[i]);}
  else{const s=S.sel;S.sel=i;setSmp(id);S.sel=s;renderTabs();renderRoll();}}
function libRemove(id){const l=S.lib.find(x=>x.id===id);if(!l)return;S.lib=S.lib.filter(x=>x!==l);
  S.rows.forEach(r=>{if(r.smp===id)r.smp='';});S.tracks.forEach(t=>{if(t.smp===id)t.smp='';});delete bufs[id];
  idb('readwrite',s=>s.delete(id)).catch(()=>{});renderRhythm();renderRoll();renderLib();save();msg('« '+l.name+' » supprimé de la bibliothèque');}
const fdur=id=>bufs[id]?bufs[id].duration.toFixed(2).replace('.',',')+' s':'absent';
function renderLib(){if(!$('#libF'))return;const fs=libFolders(),n=f=>S.lib.filter(l=>l.f===f).length,q=($('#libQ').value||'').toLowerCase();
  const nb=f=>n(f)+(f==='Synth'?1:0);
  $('#libF').innerHTML=`<button class="lf${libCur?'':' on'}" data-lf="" style="--cc:${catC('Tous')}"><span class="fe">${iconSVG('Tous',18)}</span> Tous <small>${S.lib.length+1}</small></button>`+fs.map(f=>`<button class="lf${f===libCur?' on':''}${nb(f)?'':' vide'}" data-lf="${esc(f)}" style="--cc:${catC(f)}" title="Glisse des sons (ou un dossier) ici pour les ranger dans ${esc(f)}"><span class="fe">${iconSVG(f,18)}</span> <span class="fn">${esc(f)}</span> <small>${nb(f)||''}</small>${!LIBF.includes(f)&&!n(f)?'<b data-lfx="'+esc(f)+'" title="Supprimer ce dossier vide">✕</b>':''}</button>`).join('');
  const list=S.lib.filter(l=>(!libCur||l.f===libCur)&&(!q||l.name.toLowerCase().includes(q)));
  const dest=`<option value="">Mettre sur…</option><optgroup label="Batterie">${ROWS.map((r,i)=>`<option value="r${i}">${r}</option>`).join('')}</optgroup><optgroup label="Mélodie">${S.tracks.map((t,i)=>`<option value="t${i}">${esc(t.name)}</option>`).join('')}</optgroup>`;
  const used=id=>[...S.rows.map((r,i)=>r.smp===id?ROWS[i]:''),...S.tracks.map(t=>t.smp===id?t.name:'')].filter(Boolean).join(', ');
  const pianoOn=(!libCur||libCur==='Synth')&&(!q||'piano'.includes(q)),pu=S.tracks.filter(t=>t.smp==='piano').map(t=>t.name).join(', ');
  const pianoRow=pianoOn?`<div class="li base" style="--cc:${catC('Piano')}"><button data-lpl="piano" title="Écouter">▶</button>${iconSVG('Piano',20)}<span class="ln">Piano (son de base)<small>interne, toujours disponible${pu?' · sur '+esc(pu):''}</small></span><select data-la="piano" title="Utiliser le piano sur une piste mélodique"><option value="">Mettre sur…</option>${S.tracks.map((t,i)=>`<option value="t${i}">${esc(t.name)}</option>`).join('')}</select></div>`:'';
  $('#libL').innerHTML=pianoRow+(list.length?list.map(l=>`<div class="li" style="--cc:${catC(l.f)}"><button data-lpl="${l.id}" title="Écouter">▶</button>${iconSVG(l.f,20)}<span class="ln" title="${esc(l.name)}">${esc(l.name)}<small>${fdur(l.id)}${used(l.id)?' · sur '+esc(used(l.id)):''}</small></span>`+
    `<select data-lm="${l.id}" title="Ranger dans un autre dossier">${fs.map(f=>`<option value="${esc(f)}"${optC(f)}${f===l.f?' selected':''}>■ ${esc(f)}</option>`).join('')}</select><select data-la="${l.id}" title="Utiliser ce son sur une ligne ou une piste">${dest}</select>`+
    `<button data-led="${l.id}" title="Éditer ce son : couper, reverse, fondus… (vue FM‑1, EDIT)">✂</button><button data-lr="${l.id}" title="Renommer">✎</button><button class="x${libDel===l.id?' sure':''}" data-ld="${l.id}" title="Supprimer">${libDel===l.id?'Supprimer ?':'✕'}</button></div>`).join('')
    :pianoOn?'':`<p class="lempty">${libCur?'Dossier « '+esc(libCur)+' » vide.':'Bibliothèque vide.'} Glisse tes fichiers audio ici ou clique sur « ＋ Ajouter des samples ».</p>`);}
const isAudio=f=>/^audio\//.test(f.type)||/\.(wav|aiff?|mp3|ogg|flac|m4a)$/i.test(f.name);
// Fichiers d'un glisser-déposer, dossiers compris (chaque fichier garde son chemin pour deviner son dossier)
async function dropFiles(dt){const items=[...(dt.items||[])].map(i=>i.webkitGetAsEntry&&i.webkitGetAsEntry()).filter(Boolean);
  if(!items.length)return[...dt.files].filter(isAudio);const out=[];
  const walk=async(en,path)=>{if(en.isFile){const f=await new Promise(r=>en.file(r,()=>r(null)));if(f&&isAudio(f)){try{Object.defineProperty(f,'path',{value:path+f.name});}catch(e){}out.push(f);}}
    else if(en.isDirectory){const rd=en.createReader();let batch;do{batch=await new Promise(r=>rd.readEntries(r,()=>r([])));for(const c of batch)await walk(c,path+en.name+'/');}while(batch.length);}};
  for(const en of items)await walk(en,'');return out;}
function libInit(){
  $('#libF').addEventListener('click',e=>{const x=e.target.closest('[data-lfx]');if(x){S.libF=(S.libF||[]).filter(f=>f!==x.dataset.lfx);if(libCur===x.dataset.lfx)libCur='';renderLib();save();return;}
    const b=e.target.closest('[data-lf]');if(b){libCur=b.dataset.lf;libDel='';renderLib();}});
  $('#libL').addEventListener('click',e=>{const d=e.target.dataset;
    if(d.lpl)preview(d.lpl);
    else if(d.led){M.eid=d.led;setView('fm1');goPage('sedit');say('Édition de « '+(S.lib.find(x=>x.id===d.led)||{}).name+' »');}
    else if(d.lr){const l=S.lib.find(x=>x.id===d.lr),n=prompt('Nouveau nom du son',l.name);if(n&&n.trim()){l.name=n.trim().slice(0,40);renderLib();renderRhythm();renderSnd();save();}}
    else if(d.ld){if(libDel===d.ld){libDel='';libRemove(d.ld);}else{libDel=d.ld;renderLib();}}});
  $('#libL').addEventListener('change',e=>{const d=e.target.dataset;
    if(d.lm){const l=S.lib.find(x=>x.id===d.lm);l.f=e.target.value;renderLib();renderRhythm();renderSnd();save();msg('« '+l.name+' » rangé dans '+l.f);}
    else if(d.la&&e.target.value){libAssign(d.la,e.target.value);renderLib();}});
  $('#libQ').oninput=renderLib;
  $('#libNew').onclick=()=>{const n=(prompt('Nom du nouveau dossier')||'').trim().slice(0,24);if(!n)return;if(libFolders().some(f=>f.toLowerCase()===n.toLowerCase()))return msg('Ce dossier existe déjà');
    (S.libF=S.libF||[]).push(n);libCur=n;renderLib();save();msg('Dossier « '+n+' » créé : ajoute-y tes sons');};
  $('#libAdd').onclick=()=>$('#libIn').click();$('#libDir').onclick=()=>$('#dirIn').click();
  const take=fl=>[...fl].filter(isAudio);
  $('#libIn').onchange=e=>{addFiles(take(e.target.files),false,libCur);e.target.value='';};
  $('#dirIn').onchange=e=>{addFiles(take(e.target.files),false,'');e.target.value='';};
  const z=$('#libS');z.addEventListener('dragover',e=>{e.preventDefault();z.classList.add('drag');});
  z.addEventListener('dragleave',e=>{if(!z.contains(e.relatedTarget))z.classList.remove('drag');});
  z.addEventListener('drop',async e=>{e.preventDefault();z.classList.remove('drag');const fl=await dropFiles(e.dataTransfer);if(fl.length)addFiles(fl,false,libCur);});
  // Déposer sur un dossier : les sons y sont rangés directement
  const F=$('#libF');F.addEventListener('dragover',e=>{const b=e.target.closest('[data-lf]');if(!b)return;e.preventDefault();F.querySelectorAll('.dz').forEach(x=>x.classList.remove('dz'));b.classList.add('dz');});
  F.addEventListener('dragleave',e=>{const b=e.target.closest('[data-lf]');if(b&&!b.contains(e.relatedTarget))b.classList.remove('dz');});
  F.addEventListener('drop',async e=>{const b=e.target.closest('[data-lf]');if(!b)return;e.preventDefault();e.stopPropagation();b.classList.remove('dz');z.classList.remove('drag');
    const f=b.dataset.lf,fl=await dropFiles(e.dataTransfer);if(!fl.length)return msg('Aucun fichier audio dans ce dépôt');libCur=f;await addFiles(fl,false,f);msg(fl.length+' son(s) rangé(s) dans '+(f?f:'leurs dossiers (devinés d’après le nom)'));});
  renderLib();}
