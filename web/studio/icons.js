/* ---------- Icônes des instruments (dessinées, 24×24) + une couleur par catégorie ----------
   Mêmes dessins partout : bibliothèque et menus (SVG), écran du FM-1 (canvas, Path2D).
   f = rempli couleur · d = rempli sombre · s = trait couleur · k = trait sombre · w = rempli clair */
const ICONS={
  Kick:{c:'#ff5a5a',p:[['f','M12 2.5a9.5 9.5 0 1 1 0 19a9.5 9.5 0 1 1 0-19Z'],['k','M12 6.5a5.5 5.5 0 1 1 0 11a5.5 5.5 0 1 1 0-11Z',1.6],['d','M11 3h2v2.4h-2zM11 18.6h2V21h-2zM3 11h2.4v2H3zM18.6 11H21v2h-2.4z'],['d','M10.5 10.5h3v3h-3z']]},
  Snare:{c:'#ff8a1f',p:[['f','M3 9h18v8.5H3z'],['w','M3 9a9 2.6 0 1 0 18 0a9 2.6 0 1 0-18 0Z'],['k','M3 13.5l2-2 2 4 2-4 2 4 2-4 2 4 2-4 2 4 2-2',1.3],['s','M4.5 2.5l6.5 5.5M19.5 2.5L13 8',1.8]]},
  Clap:{c:'#ff6fb5',p:[['f','M7.5 21.5C5 20.3 4 18 4 15.5V9.6a1.6 1.6 0 0 1 3.2 0V12V5.2a1.6 1.6 0 0 1 3.2 0V11V4.2a1.6 1.6 0 0 1 3.2 0V11V6.2a1.6 1.6 0 0 1 3.2 0V15c0 3.6-2.4 6.5-6 6.5Z'],['s','M19 4.5l2.5-1.5M19.8 8.5h2.7M17.5 2.5l.8-2',1.6]]},
  'Hi-hat':{c:'#ffd23f',p:[['s','M12 12.5V22M8.5 22h7',1.6],['f','M2 9.5c4-2.6 16-2.6 20 0c-4 1.4-16 1.4-20 0Z'],['f','M2 11.6c4 2.6 16 2.6 20 0c-4-1.4-16-1.4-20 0Z'],['d','M11.2 8.3h1.6v1.6h-1.6z']]},
  'Open hat':{c:'#c6f432',p:[['s','M12 14V22M8.5 22h7',1.6],['f','M2 6c4-2.6 16-2.6 20 0c-4 1.4-16 1.4-20 0Z'],['f','M2 13.5c4 2.6 16 2.6 20 0c-4-1.4-16-1.4-20 0Z'],['s','M4 9.6l-1.5 1M20 9.6l1.5 1M12 8.4v2.2',1.4]]},
  Perc:{c:'#3ddc84',p:[['s','M8 11.5l-1.2 9.5M16.5 14.5l1 7',2],['f','M8 2.5a4 4.8 0 1 1 0 9.6a4 4.8 0 1 1 0-9.6Z'],['f','M16.5 5.2a4 4.8 0 1 1 0 9.6a4 4.8 0 1 1 0-9.6Z'],['d','M7 6h1.4v1.4H7zM9 8h1.4v1.4H9zM15.5 9h1.4v1.4h-1.4zM17.5 11h1.4v1.4h-1.4z']]},
  Tom:{c:'#ff9e6b',p:[['f','M3.5 8.5h17V18a8.5 2.6 0 0 1-17 0Z'],['w','M3.5 8.5a8.5 2.6 0 1 0 17 0a8.5 2.6 0 1 0-17 0Z'],['k','M7 11.2v8.4M12 11.6v9.2M17 11.2v8.4',1.3],['s','M20 2l-5 5',1.8]]},
  Basse:{c:'#4f8cff',p:[['s','M10.5 13.5L20.5 3.5',2.6],['f','M18.8 1.5h3.7v3.7h-3.7z'],['f','M2.5 16.5a5 4.2 -45 1 0 10 0a5 4.2 -45 1 0 -10 0Z'],['k','M8 16.5l3-3M6 18.5l1.5-1.5',1.2],['d','M5.6 14.8h1.6v1.6H5.6z']]},
  Synth:{c:'#35e3c4',p:[['f','M1.5 6.5h21v12h-21z'],['k','M5.7 13v5.5M9.9 13v5.5M14.1 13v5.5M18.3 13v5.5',1.1],['d','M4.6 6.5h2.2V13H4.6zM8.8 6.5H11V13H8.8zM13 6.5h2.2V13H13zM17.2 6.5h2.2V13h-2.2z']]},
  Piano:{c:'#fff3c4',p:[['f','M2 4.5h20v15H2z'],['k','M6 12v7.5M10 12v7.5M14 12v7.5M18 12v7.5',1.1],['d','M5 4.5h2V12H5zM9 4.5h2V12H9zM15 4.5h2V12h-2zM13 4.5h.8V12H13zM19 4.5h1.5V12H19z']]},
  Nappe:{c:'#a78bfa',p:[['s','M2 7.5c2.5-3 5 3 7.5 0s5 3 7.5 0 3.5 0 5 0M2 12.5c2.5-3 5 3 7.5 0s5 3 7.5 0 3.5 0 5 0M2 17.5c2.5-3 5 3 7.5 0s5 3 7.5 0 3.5 0 5 0',2]]},
  FX:{c:'#e66bff',p:[['f','M12 1.5l2.7 6.8 7.3.5-5.6 4.7 1.8 7.1L12 16.7l-6.2 3.9 1.8-7.1L2 8.8l7.3-.5Z'],['d','M11 9.5h2v3h-2z']]},
  Voix:{c:'#8fd3ff',p:[['f','M8.8 5a3.2 3.2 0 0 1 6.4 0v6.5a3.2 3.2 0 0 1-6.4 0Z'],['s','M5.5 11a6.5 6.5 0 0 0 13 0M12 17.5v3.5M8 21.5h8',1.8],['k','M9.5 6.5h5M9.5 9h5',1]]},
  Boucles:{c:'#a3e635',p:[['s','M4 12a8 8 0 0 1 13.6-5.6M20 12a8 8 0 0 1-13.6 5.6',2.2],['f','M19.5 2.5v6.2h-6.2zM4.5 21.5v-6.2h6.2z']]},
  Samples:{c:'#9aa4b1',p:[['f','M2.5 4.5h19v15h-19z'],['k','M4.5 12h2l1.2-4 2.2 8 2-10 2 12 2-8 1.2 4h2.4',1.5]]},
  Dossier:{c:'#c0c6cf',p:[['f','M2 5.5h7l2.2 2.2H22v11.8H2z'],['d','M2 9h20v1H2z']]},
  Interne:{c:'#eef1f5',p:[['s','M2 12c2.3-7 5.7-7 8 0s5.7 7 8 0c.8-2.4 2-3.6 4-3.6',2.2]]},
  Tous:{c:'#ffd23f',p:[['f','M3 3h8v8H3zM13 3h8v8h-8zM3 13h8v8H3zM13 13h8v8h-8z']]}
};
const catC=f=>(ICONS[f]||ICONS.Dossier).c;
const icoOf=f=>ICONS[f]?f:'Dossier';
const ROWCAT=['Kick','Snare','Clap','Hi-hat','Open hat','Perc','Tom','Perc'],TYPCAT={bass:'Basse',synth:'Synth',pad:'Nappe',fx:'FX'};
// SVG pour les pages HTML
function iconSVG(f,sz){const I=ICONS[icoOf(f)],c=I.c,dk='rgba(10,11,13,.72)',wt='rgba(255,255,255,.45)';
  return `<svg class="ico" viewBox="0 0 24 24" width="${sz||18}" height="${sz||18}" aria-hidden="true">`+I.p.map(([t,d,w])=>
    t==='f'?`<path d="${d}" fill="${c}"/>`:t==='d'?`<path d="${d}" fill="${dk}"/>`:t==='w'?`<path d="${d}" fill="${wt}"/>`:
    `<path d="${d}" fill="none" stroke="${t==='s'?c:dk}" stroke-width="${w||1.6}" stroke-linecap="round" stroke-linejoin="round"/>`).join('')+'</svg>';}
// Même dessin sur l'écran du FM-1 (canvas)
const P2D={};
function drawIcon(g,f,x,y,sz){const I=ICONS[icoOf(f)];g.save();g.translate(x,y);const k=(sz||14)/24;g.scale(k,k);g.lineCap='round';g.lineJoin='round';
  I.p.forEach(([t,d,w])=>{const p=P2D[d]||(P2D[d]=new Path2D(d));
    if(t==='f'||t==='d'||t==='w'){g.fillStyle=t==='f'?I.c:t==='d'?'rgba(10,11,13,.72)':'rgba(255,255,255,.45)';g.fill(p);}
    else{g.strokeStyle=t==='s'?I.c:'rgba(10,11,13,.72)';g.lineWidth=w||1.6;g.stroke(p);}});
  g.restore();}

// Couleur d'une piste partout dans l'app : p 0-7 = lignes de batterie, 8-13 = pistes mélodiques
const trkC=p=>catC(p<8?ROWCAT[p]:TYPCAT[S.tracks[p-8].type]);
