#pragma once
// Auto-generated embed of src/web/index.html
static const char WEB_ROOT_INDEX[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>MP3 播放器</title>
  <style>
    body{font-family:Arial,Helvetica,sans-serif;margin:12px}
    .player{max-width:820px}
    .controls{display:flex;align-items:center;gap:8px;margin-bottom:8px}
    .btn{background:#eee;border:1px solid #ccc;padding:8px;border-radius:6px;cursor:pointer}
    .btn svg{vertical-align:middle}
    .btn[data-label]{display:flex;flex-direction:column;align-items:center;padding:6px 8px}
    .btn[data-label]::after{content:attr(data-label);display:block;font-size:12px;margin-top:4px;color:#333}
    .progress{width:100%;margin:8px 0}
    table{border-collapse:collapse;width:100%}
    td,th{padding:6px;border-bottom:1px solid #ddd}
    .current{background:#f0f8ff}
  </style>
</head>
<body>
  <h2>MP3 播放器</h2>
  <p id="device"></p>
  <div class="player">
    <div class="controls" id="controls">
      <button class="btn" id="btnPrev" title="上一曲" data-label="上一曲">
        <svg width="20" height="20" viewBox="0 0 24 24"><path d="M11 18V6L3 12l8 6zM20 6v12h-2V6h2z"/></svg>
      </button>
      <button class="btn" id="btnPlay" title="播放" data-label="播放">
        <svg width="20" height="20" viewBox="0 0 24 24" id="iconPlay"><path d="M8 5v14l11-7z"/></svg>
      </button>
      <button class="btn" id="btnPause" title="暂停" data-label="暂停">
        <svg width="20" height="20" viewBox="0 0 24 24"><path d="M6 19h4V5H6v14zm8-14v14h4V5h-4z"/></svg>
      </button>
      <button class="btn" id="btnStop" title="停止" data-label="停止">
        <svg width="20" height="20" viewBox="0 0 24 24"><path d="M6 6h12v12H6z"/></svg>
      </button>
      <button class="btn" id="btnNext" title="下一曲" data-label="下一曲">
        <svg width="20" height="20" viewBox="0 0 24 24"><path d="M13 12l8-6v12l-8-6zM4 6v12h2V6H4z"/></svg>
      </button>
      <div id="loopControls" style="display:inline-flex;gap:6px;align-items:center">
        <button class="btn" id="loop0" title="顺序播放" data-label="顺序">⟲</button>
        <button class="btn" id="loop1" title="单曲循环" data-label="单曲">🔁</button>
        <button class="btn" id="loop2" title="倒序播放" data-label="倒序">↺</button>
        <button class="btn" id="loop3" title="随机播放" data-label="随机">🎲</button>
      </div>
      音量： <input id="vol" type="range" min="0" max="21" value="12">
    </div>
    <div>
      <div id="now" style="font-weight:600">加载中…</div>
      <input id="prog" class="progress" type="range" min="0" max="100" value="0">
      <div style="display:flex;justify-content:space-between;font-size:12px"><span id="pos">0:00</span><span id="dur">0:00</span></div>
    </div>
  </div>

  <h3>播放列表</h3>
  <div id="playlist">加载中…</div>
  <div id="status" style="margin-top:8px;color:#333"></div>

  <script>
  function escapeHtml(s){ return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;'); }

  async function fetchList(){
    try{
      const r = await fetch('/api/list');
      const arr = await r.json();
      const cur = window.current || '';
      let html = '<table><tr><th>#</th><th>Title</th><th>Actions</th></tr>';
      for(let i=0;i<arr.length;i++){
        const name = arr[i];
        const safe = escapeHtml(name);
        const cls = (name===cur)?' class="current"':'';
        html += `<tr${cls}><td>${i+1}</td><td>${safe}</td><td>`+
                `<button onclick="playIndex(${i})" class="btn">▶</button>`+
                ` <button onclick="(async()=>{await fetch('/music?file='+encodeURIComponent(name));})()" class="btn">🔊</button>`+
                `</td></tr>`;
      }
      html += '</table>';
      document.getElementById('playlist').innerHTML = html;
    }catch(e){ console.error('fetchList error', e); }
  }

  function playFile(fn){ fetch('/api/play?file='+encodeURIComponent(fn)).then(()=>status()); }
  function playIndex(i){ fetch('/api/play_index?i='+i).then(()=>status()); }

  let dragging = false;
  let lastDurationSec = 0;
  async function status(){
    const r = await fetch('/api/status');
    const j = await r.json();
    window.current = j.current || '';
    document.getElementById('now').innerText = j.current || '—';
    document.getElementById('vol').value = j.volume;
    const lm = (typeof j.loop_mode !== 'undefined') ? j.loop_mode : 0;
    ['loop0','loop1','loop2','loop3'].forEach((id,idx)=>{
      const el = document.getElementById(id);
      if(!el) return;
      if(idx === lm) el.style.outline = '2px solid #66a'; else el.style.outline = 'none';
    });
    document.getElementById('device').innerText = '设备 IP：' + (location.hostname || '');
    const pos = j.position_ms || 0;
    const dur = j.duration_ms || 0;
    lastDurationSec = Math.floor(dur/1000);
    const percent = dur? Math.floor((pos/dur)*100):0;
    if(!dragging){
      document.getElementById('prog').value = percent;
      document.getElementById('pos').innerText = formatMs(pos);
    }
    // always update duration display (helpful even while dragging)
    document.getElementById('dur').innerText = formatMs(dur);
    fetchList();
  }

  function formatMs(ms){
    const s = Math.floor(ms/1000);
    const m = Math.floor(s/60);
    const sec = s%60;
    return m+':'+String(sec).padStart(2,'0');
  }

  document.getElementById('btnPlay').addEventListener('click', ()=>fetch('/api/play_current').then(()=>status()));
  document.getElementById('btnPause').addEventListener('click', ()=>fetch('/api/pause').then(()=>status()));
  document.getElementById('btnStop').addEventListener('click', ()=>fetch('/api/stop').then(()=>status()));
  document.getElementById('btnPrev').addEventListener('click', ()=>fetch('/api/prev').then(()=>status()));
  document.getElementById('btnNext').addEventListener('click', ()=>fetch('/api/next').then(()=>status()));
  document.getElementById('vol').addEventListener('input', e=>fetch('/api/volume?val='+e.target.value));
  function setLoopMode(m){ fetch('/api/loop?mode='+m).then(()=>status()).catch(e=>console.error(e)); }
  ['loop0','loop1','loop2','loop3'].forEach((id,idx)=>{ const el=document.getElementById(id); if(el) el.addEventListener('click', ()=>setLoopMode(idx)); });

  const prog = document.getElementById('prog');
  // use pointer events so mouse/touch work consistently
  prog.addEventListener('pointerdown', ()=>{ dragging = true; });
  // while dragging, update the shown position based on current slider value
  prog.addEventListener('pointermove', ()=>{
    if(!dragging) return;
    const pct = parseInt(prog.value);
    if(lastDurationSec){
      const sec = Math.round((pct/100)*lastDurationSec);
      document.getElementById('pos').innerText = formatMs(sec*1000);
    }
  });
  window.addEventListener('pointerup', async (e)=>{
    if(!dragging) return;
    dragging = false;
    const pct = parseInt(prog.value);
    if(!lastDurationSec){
      // cannot seek unknown-duration tracks
      console.warn('seek ignored: unknown duration');
      status();
      return;
    }
    const sec = Math.round((pct/100)*lastDurationSec);
    try{ await fetch('/api/seek?sec='+sec); }catch(e){ console.error('seek failed', e); }
    status();
  });

  status(); setInterval(status,1000);
  </script>
</body>
</html>
)rawliteral";
