import subprocess 
import time 
import os 

repetitions = 5

for exe in list(os.listdir("./build_archive/")) + ["python3"]:
    if exe == ".DS_Store": 
        continue 

    if exe != "python3":
        exe_full = "./" + exe
    else:
        exe_full = exe 

    for batch in os.listdir("./benchmarks/"):
        dir = os.path.join("./benchmarks/", batch)
        if not os.path.isdir(dir):
            continue 

        for script in os.listdir(dir):
            if not script.endswith(".py"): continue 
            script_orig = script 
            script = os.path.join("..", dir, script)
            
            os.chdir("./build_archive/")
            start_time = time.time()
            for x in range(0, repetitions):
                p = subprocess.Popen([exe_full, script], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                p.wait()
            print("%s,%s/%s,%f" % (exe, batch, script_orig, (time.time() - start_time)/float(repetitions)))
            
            os.chdir("..")
