# QuarkOS
An hobby OS experiment on trying to combine Windows's PE applications with Linux's ELF applications, it features a hybrid kernel(Half monolithic, Half modular)

# Building
1. Clone the project to an directory.
2. Optional: download my build tool QuarkStrap (currently only automates generating limine config and adding module paths to it).
3. If using QuarkStrap just run ```QuarkStrap <Project Path> --setup-docker``` (```--setup-docker``` is only needed for the first build, you can remove this option after the first build).
4. If not using QuarkStrap, firstly run ```builddocker.bat```, then just run ```run.bat```.
5. The ISO will appear in the root directory of the project.
