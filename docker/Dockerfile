# Padavan Firmware Build Environment
# Based on Ubuntu 20.04 LTS for MIPS cross-compilation
#
# Usage:
#   docker build -t padavan-builder .
#   docker run --rm -v $(pwd):/src padavan-builder ./build_firmware_modify PSG1218

FROM ubuntu:20.04

LABEL maintainer="padavan-builder"
LABEL description="Build environment for rt-n56u/padavan router firmware"

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Shanghai

# Install build dependencies
# Build essentials
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    gcc \
    g++ \
    make \
    cmake \
    autoconf \
    automake \
    libtool \
    libtool-bin \
    pkg-config \
    # Cross-compilation tools
    flex \
    bison \
    gawk \
    gperf \
    # Libraries
    zlib1g-dev \
    libgmp3-dev \
    libmpc-dev \
    libmpfr-dev \
    libncurses5-dev \
    libltdl-dev \
    # Documentation tools
    python3 \
    python3-docutils \
    gettext \
    autopoint \
    texinfo \
    help2man \
    # Compression utilities
    unzip \
    xxd \
    cpio \
    wget \
    curl \
    git \
    # Shell check for CI
    shellcheck \
    # Archive tools
    p7zip-full \
    md5deep \
    # Misc
    fakeroot \
    kmod \
    file \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Create working directory
WORKDIR /src

# Set up environment variables
ENV ROOTDIR=/src/trunk
ENV PATH=/src/toolchain-mipsel/toolchain-mipsel/bin:${PATH}

# Copy project files (will be overridden by volume mount)
COPY . .

# Download and prepare toolchain if not exists
RUN if [ ! -d "toolchain-mipsel/toolchain-mipsel" ]; then \
        cd toolchain-mipsel && \
        sh dl_toolchain.sh && \
        cd ..; \
    fi

# Default command
CMD ["/bin/bash"]
