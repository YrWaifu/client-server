import subprocess

def run_client():
    command = ['./client', '--ip-address', '127.0.0.1', '--port', '12345']
    
    try:
        with subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True) as process:
            def send_input(data):
                process.stdin.write(data)
                process.stdin.flush()

            def read_output():
                output = process.stdout.readline().strip()
                while output == "":
                    output = process.stdout.readline().strip()
                return output

            # Отправка начальных данных для входа и регистрации
            send_input("2\n")  # Выбираем логин
            send_input("1\n")  # Вводим имя пользователя
            send_input("1\n")  # Вводим пароль
            
            # Ожидание завершения входа
            while True:
                output = read_output()
                if "Press the button:" not in output:
                    break
            
            # Подключение к каналу
            send_input("/join channel3\n")
            while True:
                output = read_output()
                if "connected successfully" in output:
                    break

            # Отправка команд read и чтение ответов
            for i in range(5):
                send_input('/read\n')
                print(f"{i} MESSAGE\n")
                for i in range(7):
                    output = read_output()
                    print(f"{output}")
                

            # Завершаем работу с программой
            send_input('/exit\n')
            process.wait()

    except subprocess.CalledProcessError as e:
        print(f"Произошла ошибка при выполнении команды: {e}")
    except Exception as e:
        print(f"Неожиданная ошибка: {e}")

if __name__ == "__main__":
    run_client()
