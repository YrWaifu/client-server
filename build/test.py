import subprocess
import time
import select

command = ['./client', '--ip-address', '127.0.0.1', '--port', '5555']

process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, bufsize=1, text=True)

process.stdin.write('/join channel3\n')
process.stdin.flush()

try:
    while True:
        process.stdin.write('asd\n')
        process.stdin.flush()
        
        ready, _, _ = select.select([process.stdout], [], [], 1)
        if ready:
            output = process.stdout.readline().strip()
            if output:
                print(output)
        
        time.sleep(1)
except KeyboardInterrupt:
    process.terminate()
    process.wait()
