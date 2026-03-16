#pragma once
// Auto-generated embed of src/web/index.html
static const char WEB_ROOT_INDEX[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>MP3 Player</title>
  <style>
    body{font-family:Arial,Helvetica,sans-serif;margin:12px}
    table{border-collapse:collapse;width:100%}
    td,th{padding:6px;border-bottom:1px solid #ddd}
    button{margin:2px;padding:6px}
    #controls{margin-bottom:8px}
    .current{background:#f0f8ff}
  </style>
</head>
<body>
  <h2>MP3 Player</h2>
  <p id="device"></p>
  <div id="controls">
    <button id="btnPlay">Play</button>
    <button id="btnStop">Stop</button>
    <button id="btnPrev">Prev</button>
    <button id="btnNext">Next</button>
    Loop: <input type="checkbox" id="loopbox">
    Volume: <input id="vol" type="range" min="0" max="21" value="12">
  </div>

  <h3>Playlist</h3>
  <div id="playlist">加载中…</div>
  <div id="status" style="margin-top:8px;color:#333"></div>

  <script>
  function escapeHtml(s){ return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;'); }

  async function fetchList(){
    const r = await fetch('/api/list');
    const arr = await r.json();
    const cur = window.current || '';
    let html = '<table><tr><th>#</th><th>Title</th><th>Actions</th></tr>';
    for(let i=0;i<arr.length;i++){
      const name = arr[i];
      const safe = escapeHtml(name);
      const cls = (name===cur)?' class="current"':'';
      html += `<tr${cls}><td>${i+1}</td><td>${safe}</td><td>`+
              `<button onclick="playFile('${encodeURIComponent(name)}')">Play</button>`+
              ` <a href="/music?file=${encodeURIComponent(name)}" target=_blank>Stream</a>`+
              ` <button onclick="playIndex(${i})">Play#</button>`+
              `</td></tr>`;
    }
    html += '</table>';
    document.getElementById('playlist').innerHTML = html;
  }

  function playFile(fn){ fetch('/api/play?file='+fn).then(()=>status()); }
  function playIndex(i){ fetch('/api/list').then(r=>r.json()).then(arr=>{ if(i>=0 && i<arr.length) fetch('/api/play?file='+encodeURIComponent(arr[i])).then(()=>status()); }); }

  async function status(){
    const r = await fetch('/api/status');
    const j = await r.json();
    window.current = j.current || '';
    document.getElementById('status').innerText = 'Playing: ' + j.current + ' | Volume: ' + j.volume + ' | Loop: ' + j.loop;
    document.getElementById('vol').value = j.volume;
    document.getElementById('loopbox').checked = j.loop;
    document.getElementById('device').innerText = 'Device IP: ' + (location.hostname || '');
    fetchList();
  }

  document.getElementById('btnPlay').addEventListener('click', ()=>fetch('/api/play_current').then(()=>status()));
  document.getElementById('btnStop').addEventListener('click', ()=>fetch('/api/stop').then(()=>status()));
  document.getElementById('btnPrev').addEventListener('click', ()=>fetch('/api/prev').then(()=>status()));
  document.getElementById('btnNext').addEventListener('click', ()=>fetch('/api/next').then(()=>status()));
  document.getElementById('vol').addEventListener('input', e=>fetch('/api/volume?val='+e.target.value));
  document.getElementById('loopbox').addEventListener('change', e=>fetch('/api/toggleLoop?val='+(e.target.checked?1:0)).then(()=>status()));

  status(); setInterval(status,3000);
  </script>
</body>
</html>
)rawliteral";
