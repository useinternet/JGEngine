// RunEditor.cmd check (DevOps, 2026-10-17). Calls the script the way a dashboard server does - from Node through
// pipes - and goes through: start, already running, a build waiting for the running editor, MainTree hold (busy),
// a game project editor while the main tree is held, -wait until the hold ends, fresh build output (busy).
// Every editor it starts is closed with WM_CLOSE. Needs a free main tree; stops at once otherwise.
//   node runeditor_check.js [ENGINE_ROOT] [GAME_PROJECT_DIR]
const { spawn, execFileSync } = require('child_process');
const fs = require('fs');
const path = require('path');

const root = process.argv[2] || 'C:\\Develop\\JGEngine';
const project = process.argv[3] || path.join(root, 'Temp', 'Games', 'GameUIDemo');
const runEditor = path.join(root, 'Build', 'BatchFiles', 'RunEditor.cmd');
const mainTree = path.join(root, 'Build', 'BatchFiles', 'MainTree.cmd');
const mainLauncher = path.join(root, 'Bin', 'DevelopEngine', 'JGLauncher.exe');
const projectLauncher = path.join(project, 'Bin', 'DevelopEngine', 'JGLauncher.exe');
let failures = 0;

function log(text) {
    const now = new Date();
    const time = now.toTimeString().slice(0, 8);
    console.log(`[${time}] ${text}`);
}

// Runs a .cmd through cmd.exe with piped output. Resolves when every pipe is closed - a child that inherited a
// pipe (the editor, if it did) would keep 'close' from coming, so exit and close times are both reported.
function runCmd(file, args, limitMs = 120000) {
    return new Promise((resolve) => {
        const started = Date.now();
        const child = spawn('cmd.exe', ['/d', '/s', '/c', `""${file}" ${args}"`], { windowsVerbatimArguments: true, windowsHide: true });
        let out = '';
        let exitMs = -1;
        let code = null;
        child.stdout.on('data', (data) => { out += data.toString('utf8'); });
        child.stderr.on('data', (data) => { out += data.toString('utf8'); });
        child.on('exit', (exitCode) => { exitMs = Date.now() - started; code = exitCode; });
        const timer = setTimeout(() => { resolve({ code, out: out.trim(), exitMs, closeMs: -1 }); }, limitMs);
        child.on('close', () => {
            clearTimeout(timer);
            resolve({ code, out: out.trim(), exitMs, closeMs: Date.now() - started });
        });
    });
}

function powershell(script) {
    return execFileSync('powershell.exe', ['-NoProfile', '-NonInteractive', '-Command', script], { encoding: 'utf8' }).trim();
}

function launchers(launcher) {
    const text = powershell(`Get-CimInstance Win32_Process -Filter "Name = 'JGLauncher.exe'" | ForEach-Object { '{0}|{1}' -f $_.ProcessId, $_.ExecutablePath }`);
    return text.split(/\r?\n/).filter((line) => line !== '').map((line) => line.split('|')).filter((parts) => parts[1] && parts[1].toLowerCase() === launcher.toLowerCase()).map((parts) => Number(parts[0]));
}

// Waits for the editor window, lets it run a moment, sends WM_CLOSE and reports the exit code.
function closeEditor(pid) {
    return powershell(`$p = Get-Process -Id ${pid} -ErrorAction SilentlyContinue; if ($null -eq $p) { 'not running' } else { $null = $p.Handle; $deadline = (Get-Date).AddSeconds(60); while ($p.MainWindowHandle -eq 0 -and -not $p.HasExited -and (Get-Date) -lt $deadline) { Start-Sleep -Milliseconds 500; $p.Refresh() }; Start-Sleep -Seconds 3; $p.Refresh(); $title = $p.MainWindowTitle; [void]$p.CloseMainWindow(); if ($p.WaitForExit(60000)) { 'closed, window "' + $title + '", exit ' + $p.ExitCode } else { 'did not close in 60s' } }`);
}

function expect(name, result, code, pattern) {
    const ok = result.code === code && (pattern === undefined || pattern.test(result.out)) && result.closeMs >= 0;
    if (!ok) {
        failures++;
    }
    log(`${ok ? 'PASS' : 'FAIL'} ${name}: exit ${result.code} (want ${code}), returned ${result.exitMs} ms, pipes closed ${result.closeMs} ms`);
    for (const line of result.out.split(/\r?\n/)) {
        log(`     | ${line}`);
    }
    return ok;
}

function check(name, ok, detail) {
    if (!ok) {
        failures++;
    }
    log(`${ok ? 'PASS' : 'FAIL'} ${name}${detail ? ': ' + detail : ''}`);
}

function pidOf(result) {
    const match = /pid (\d+)/.exec(result.out);
    return match ? Number(match[1]) : 0;
}

function sleep(ms) {
    return new Promise((resolve) => setTimeout(resolve, ms));
}

async function main() {
    log(`engine ${root}, game project ${project}`);
    const status = await runCmd(mainTree, '-name=DevOpsCheck status');
    log(`status: exit ${status.code} | ${status.out.replace(/\r?\n/g, ' / ')}`);
    if (status.code !== 0 || launchers(mainLauncher).length > 0) {
        log('main tree is not free - stopping');
        process.exit(2);
    }

    log('-- 1 start');
    const mainLog = path.join(root, 'Bin', 'DevelopEngine', 'jg_log.txt');
    const logBefore = fs.existsSync(mainLog) ? fs.statSync(mainLog).mtimeMs : 0;
    const started = await runCmd(runEditor, '-name=DevOpsCheck');
    expect('start', started, 0, /^에디터를 띄움\(pid \d+\)/);
    const pid = pidOf(started);
    await sleep(2000);
    const running = launchers(mainLauncher);
    check('editor runs after the script returned', running.length === 1 && running[0] === pid, `JGLauncher from main Bin: [${running.join(', ')}]`);
    check('editor writes Bin\\DevelopEngine\\jg_log.txt', fs.existsSync(mainLog) && fs.statSync(mainLog).mtimeMs > logBefore);

    log('-- 2 already running');
    const again = await runCmd(runEditor, '-name=DevOpsCheck');
    expect('already running', again, 89, new RegExp(`^이미 떠 있음 — .*pid ${pid}`));
    check('still one editor', launchers(mainLauncher).length === 1);

    log('-- 3 a MainTree hold waits for the running editor (as builds do)');
    const busyStatus = await runCmd(mainTree, '-name=DevOpsCheck status');
    expect('status names the editor', busyStatus, 1, new RegExp(`running Bin\\\\DevelopEngine\\\\JGLauncher\\.exe \\(pid ${pid}`));
    const hold = await runCmd(mainTree, '-name=DevOpsCheck -wait=0 hold -minutes=1');
    expect('hold while the editor runs', hold, 90);
    if (hold.code === 0) {
        await runCmd(mainTree, '-name=DevOpsCheck release');
    }
    log(`close: ${closeEditor(pid)}`);

    log('-- 4 busy: MainTree hold of another session');
    const held = await runCmd(mainTree, '-name=DevOpsCheckHold -wait=1 hold -minutes=3');
    expect('hold taken', held, 0);
    const busy = await runCmd(runEditor, '-name=DevOpsCheck');
    expect('busy (hold)', busy, 90, /^빌드 중 — DevOpsCheckHold: hold 3m.*끝나면 다시 눌러 주세요/);
    check('no editor started', launchers(mainLauncher).length === 0);

    log('-- 5 game project editor while the main tree is held');
    if (fs.existsSync(projectLauncher)) {
        const projectStarted = await runCmd(runEditor, `-name=DevOpsCheck -project="${project}"`);
        expect('game project start', projectStarted, 0, /^에디터를 띄움\(pid \d+\)/);
        const projectAgain = await runCmd(runEditor, `-name=DevOpsCheck -project="${project}"`);
        expect('game project already running', projectAgain, 89, /^이미 떠 있음/);
        log(`close: ${closeEditor(pidOf(projectStarted))}`);
    } else {
        log(`skipped: ${projectLauncher} not built`);
    }

    log('-- 6 -wait=40, the hold ends after about 8 s');
    const waiting = runCmd(runEditor, '-name=DevOpsCheck -wait=40');
    await sleep(8000);
    const released = await runCmd(mainTree, '-name=DevOpsCheckHold release');
    log(`release: exit ${released.code} | ${released.out}`);
    const waited = await waiting;
    expect('start after waiting', waited, 0, /^에디터를 띄움\(pid \d+\).*기다린 뒤/);
    check('waited about 8 s', waited.exitMs >= 7000 && waited.exitMs < 20000, `${waited.exitMs} ms`);
    log(`close: ${closeEditor(pidOf(waited))}`);

    log('-- 7 busy: build output written just now');
    const obj = path.join(root, 'Temp', 'Obj');
    fs.mkdirSync(obj, { recursive: true });
    const probe = path.join(obj, '_runeditor_probe.tmp');
    fs.writeFileSync(probe, 'RunEditor check\r\n');
    const fresh = await runCmd(runEditor, '-name=DevOpsCheck');
    fs.unlinkSync(probe);
    expect('busy (fresh build output)', fresh, 90, /^빌드 중 — build output written .*_runeditor_probe\.tmp/);
    check('no editor started', launchers(mainLauncher).length === 0);

    log('-- 8 a launcher that ends at once (fake game project, whoami.exe as JGLauncher.exe)');
    const fake = path.join(root, 'Temp', 'Games', '_RunEditorFake');
    fs.mkdirSync(path.join(fake, 'Bin', 'DevelopEngine'), { recursive: true });
    fs.writeFileSync(path.join(fake, '_RunEditorFake.jgproject'), '{ "Project": { "Name": "_RunEditorFake" } }\r\n');
    fs.copyFileSync(path.join(process.env.SystemRoot, 'System32', 'whoami.exe'), path.join(fake, 'Bin', 'DevelopEngine', 'JGLauncher.exe'));
    const quick = await runCmd(runEditor, `-name=DevOpsCheck -project="${fake}"`);
    fs.rmSync(fake, { recursive: true, force: true });
    expect('ended at once', quick, 93, /^에디터가 바로 끝남\(종료 코드 0\)/);

    const after = await runCmd(mainTree, '-name=DevOpsCheck status');
    log(`status: exit ${after.code} | ${after.out.replace(/\r?\n/g, ' / ')}`);
    log(failures === 0 ? 'ALL PASS' : `${failures} FAILED`);
    process.exit(failures === 0 ? 0 : 1);
}

main();
