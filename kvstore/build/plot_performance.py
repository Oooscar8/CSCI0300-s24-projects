import csv, sys

class Colors:
    SUCCESS = '\033[92m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'

def draw_bar(height, value=None, legend=None, time=None, speedup=None, color=None, char="❚"):
    output = ""
    if legend:
        output += f"{legend}\t"
    if color:
        output += f"{color}"

    output += f"{char * height}"
    
    if value:
        output += f" {value} keys/second"
    if time and speedup:
        output += f" ({time}ms, {round(speedup, 1)}x speedup)"
    elif time:
        output += f" ({time}ms)"

    if color:
        output += f"{Colors.ENDC}"
    print(output)


if __name__ == "__main__":
    path = "performance-runtime.csv"
    with open(path, 'r', newline='') as file:
        # assuming the csv file has columns: test, time
        raw_csv = csv.DictReader(file)

        # generate list containing each test's title, time, and height
        data = []
        for row in raw_csv:
            # compute height as 1/10th the # of ms
            data.append({"title": row["title"], "time": row["time"], "tput": row["tput"], "height": round(float(row["tput"]) / 4000.0)})

        if len(data) == 0:
            print(f"{Colors.FAIL}ERROR: Performance tests did not run successfully.{Colors.ENDC}")
            sys.exit(1)

        print("Performance Test Results")
        print("========================")

        for i, row in enumerate(data):
            # use no color with single threaded tests
            if i % 2 == 0:
                draw_bar(row["height"], row["tput"], row["title"], row["time"])
            else:
                # plot multithreaded tests in green if they are faster than the previous tests
                speedup = float(row["tput"]) / float(data[i-1]["tput"])
                if (speedup >= 2):
                    draw_bar(row["height"], row["tput"], row["title"], row["time"], speedup, Colors.SUCCESS)
                else:
                    draw_bar(row["height"], row["tput"], row["title"], row["time"], speedup, Colors.FAIL)
