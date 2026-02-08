# OpenBeken Build System - Replication Guide

NOTE!! - `build_tool.py` goes in the **root** folder, while the other two go in the `docker` folder.

## Prerequisites

Before you start, ensure you have:

1.  **Docker Desktop** (Installed and running)
2.  **Python 3.x** (Installed and added to PATH)
3.  **Git** (Installed and added to PATH)

## Step-by-Step Instructions

1.  **Create & Open Folder**:
    Create a new folder (e.g., `MyBuildEnv`), right-click inside it, and select "Open Terminal Here".

2.  **Clone Repository**:

    in powershell type the following command:
    git clone --recursive https://github.com/openshwprojects/OpenBK7231T_App.git

3.  **Enter Repository**:

    in powershell change directory to the repository folder
    cd OpenBK7231T_App

4.  **Copy Files** (Crucial Step):
    - Copy/replace `Dockerfile`, `build_tool.py` and `entrypoint.sh` to: `OpenBK7231T_App\docker\`
	- (The `docker` folder is already present in the repository).

    **Final Structure must look like this:**

    ```
    OpenBK7231T_App\
    
    ├── docker\
    │   ├── Dockerfile         <-- Docker config is HERE
    │   ├── entrypoint.sh      <-- Entry script is HERE
	│   └── build_tool.py      <-- Script is HERE
    ├── platforms\
    ├── src\
    └── ... (other git files)
    ```

5.  **Run Tool**:
    Run the script from the root folder:

    ```powershell
    python build_tool.py
    ```

6.  **Follow Prompts**:
    Select your platform, drivers, and version. The tool will handle the rest
    (building the Docker image automatically on the first run which will take some time on the first run only,
    subsequent runs will be much faster).

## Advanced Usage

The tool also supports command-line arguments for advanced control:

- **Check Commands**:

    ```powershell
    python build_tool.py --help
    ```

- **Force Clean Build**:
  Use this to delete temporary files and force a full rebuild (simulates `make clean`):

    ```powershell
    python build_tool.py --clean
    ```

- **Specify Flash Size**:
  Force a specific flash size (automatically adds "MB" suffix if only numbers are provided):

    ```powershell
    python build_tool.py --flash-size 4MB
    ```

- **Run from Anywhere (External Folder)**:
  If the script is not in the repo root, specify the path to the repository source:

    ```powershell
    python build_tool.py --src "C:\Users\user\Desktop\OpenBK7231T_App"
    ```

- **Rebuild Docker Image**:
  Force a rebuild of the Docker environment (e.g., after updating `entrypoint.sh` or `Dockerfile`):
    ```powershell
    python build_tool.py --no-cache
    ```

## Custom Output Paths

When prompted for the "Output Folder", you can provide:

1.  **A name**: Creates a folder inside the current directory (e.g., `test_OUT`).
2.  **An absolute path**: Saves artifacts to any external folder (e.g., `C:\Users\user\Desktop\FirmwareBuilds`).
