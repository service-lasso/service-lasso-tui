#!/usr/bin/env python3
"""Drive the real TUI through the implemented feature surface using tmux."""
import argparse, base64, json, os, pathlib, subprocess, sys, time

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--binary', required=True)
    parser.add_argument('--url', required=True)
    parser.add_argument('--token', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--ffmpeg', default=os.environ.get('FFMPEG', 'ffmpeg'))
    args = parser.parse_args()
    out = pathlib.Path(args.output).resolve(); out.mkdir(parents=True, exist_ok=True)
    socket = f'tui-e2e-{os.getpid()}'
    session = 'tui'
    tmux = ['tmux', '-L', socket]

    def run(*a): return subprocess.run([*tmux, *a], check=True, text=True, capture_output=True)
    def capture():
        text = run('capture-pane', '-p', '-e', '-t', session).stdout
        return text
    def send(*keys):
        run('send-keys', '-t', session, *keys)
    def wait_for(needle, seconds=8):
        deadline = time.monotonic() + seconds
        last = ''
        while time.monotonic() < deadline:
            last = capture()
            if needle in last: return last
            time.sleep(.1)
        raise RuntimeError(f'missing {needle!r}\n{last}')

    chunks = []; started = time.monotonic()
    def record(delay=1.2):
        raw = ('\x1b[2J\x1b[H' + capture()).encode()
        chunks.append({'t': time.monotonic() - started, 'data': base64.b64encode(raw).decode()})
        time.sleep(delay)

    failures = []; seen = []
    try:
        run('new-session', '-d', '-s', session, '-x', '120', '-y', '32',
            'env', 'TERM=xterm-256color', f'SERVICE_LASSO_API_TOKEN={args.token}',
            args.binary, '--api', args.url)
        view = wait_for('Service Lasso');
        view = wait_for('@demo-api', 15); record()
        for needle in ['SERVICE ADMIN', 'services', '@demo-api', 'RUNNING']:
            if needle not in view: failures.append(f'overview missing {needle}')

        send('Enter'); view = wait_for('DETAIL'); record()
        for tab, needle in [('2','[Dependencies]'), ('3','[Endpoints]')]:
            send(tab); view = wait_for(needle); record(1.0)
            seen.append(needle)

        # Walk the implemented journey rail and collect every screen title.
        titles = {'GRAPH','VARIABLES','ENDPOINTS','LIFECYCLE','METADATA','INBOX','QUEUE','AUDIT','LOGS'}
        for _ in range(12):
            send('Right'); time.sleep(.25); view = capture()
            for title in list(titles):
                if title in view: titles.remove(title); seen.append(title); record(.8)
            if not titles: break
        if titles: failures.append('journeys not reached: ' + ','.join(sorted(titles)))

        # Return to lifecycle and prove the server-issued preview path.
        for _ in range(12):
            send('Left'); time.sleep(.2); view = capture()
            if 'LIFECYCLE' in view: break
        if 'LIFECYCLE' not in view: failures.append('lifecycle screen not reachable')
        else:
            send('Down','Down','Down','Enter'); wait_for('Draft:')
            send('Enter'); wait_for('Confirmation phrase:'); record(1.5); seen.append('PREVIEW')

        send('q'); time.sleep(.5)
        result = {'ok': not failures, 'failures': failures, 'seen': seen,
                  'chunks': len(chunks), 'duration': time.monotonic() - started}
    except Exception as error:
        result = {'ok': False, 'failures': failures + [str(error)], 'seen': seen,
                  'chunks': len(chunks), 'duration': time.monotonic() - started}
    finally:
        subprocess.run([*tmux, 'kill-server'], capture_output=True)
        (out / 'terminal.chunks.jsonl').write_text(''.join(json.dumps(c) + '\n' for c in chunks))
        (out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
        renderer = pathlib.Path(__file__).with_name('render-terminal-video.py')
        subprocess.run([sys.executable, str(renderer), '--chunks', str(out / 'terminal.chunks.jsonl'),
                        '--output', str(out / 'video'), '--ffmpeg', args.ffmpeg], check=False)
    print(json.dumps(result))
    return 0 if result['ok'] else 1

if __name__ == '__main__':
    raise SystemExit(main())
