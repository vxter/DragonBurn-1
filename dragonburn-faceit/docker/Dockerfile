FROM mcr.microsoft.com/windows/servercore:ltsc2022

# Install build tools
RUN powershell -Command \
    Invoke-WebRequest -Uri https://aka.ms/vs/17/release/vc_redist.x64.exe -OutFile vc_redist.exe; \
    .\vc_redist.exe /install /quiet /norestart

# Copy source code
COPY . /app/

WORKDIR /app

# Build the application
RUN powershell -Command \
    mkdir build; \
    cd build; \
    cmake ..; \
    cmake --build . --config Release

# Copy webradar HTML
RUN powershell -Command \
    mkdir data; \
    copy ..\webradar.html data\

EXPOSE 8080

# Run with reduced privileges to help detection evasion
USER ContainerUser

ENTRYPOINT ["C:/app/build/Release/dragonburn-faceit.exe"]
