#!/bin/bash
# shellcheck disable=SC2155

# This script is primarily for checking and handling releases. If you are
# looking to build ECWolf then you should build manually.

# This script takes a single argument which specifies which configuration to
# use. Running with no arguments will list out config names, but you can also
# look at the bottom of this script.

# Anything that the configs write out to /results (logs, build artifacts) will
# be copied back to the hosts results directory.

# Infrastructure ---------------------------------------------------------------

# Build our clean environment if we don't have one built already
check_environment() {
	declare -n Config=$1
	shift

	declare DockerTag="${Config[dockerimage]}:${Config[dockertag]}"

	if ! docker image inspect "$DockerTag" &> /dev/null; then
		declare Dockerfile=$(mktemp -p .)
		"${Config[dockerfile]}" > "${Dockerfile}"
		docker build -t "$DockerTag" -f "$Dockerfile" . || {
			rm "$Dockerfile"
			echo 'Failed to create build environment' >&2
			return 1
		}
		rm "$Dockerfile"
	fi
	return 0
}

# Recursively build docker environments
check_environment_prereq() {
	declare ConfigName=$1
	shift

	[[ $ConfigName ]] || return 0

	declare -n Config=$ConfigName
	if [[ ${Config[prereq]} ]]; then
		check_environment_prereq "${Config[prereq]}" || return
	fi

	check_environment "$ConfigName"
}

run_config() {
	declare ConfigName=$1
	shift

	declare -n Config=$ConfigName

	check_environment_prereq "$ConfigName" || return

	declare Container
	Container=$(docker create -i -v "$(pwd):/mnt:ro" "${Config[dockerimage]}:${Config[dockertag]}" bash -s --) || return
	{
		declare -fx
		echo "\"${Config[entrypoint]}\" \"\$@\""
	} | docker start -i "$Container"
	declare Ret=$?

	if (( Ret == 0 )); then
		mkdir -p "results/${ConfigName}"
		docker cp "$Container:/results/." "results/${ConfigName}"
	fi

	docker rm "$Container" > /dev/null

	return "$Ret"
}

main() {
	declare SelectedConfig=$1
	shift

	declare ConfigName

	# List out configs
	if [[ -z $SelectedConfig ]]; then
		declare -A ConfigGroups=([all]=1)
		for ConfigName in "${ConfigList[@]}"; do
			declare -n Config=$ConfigName
			ConfigGroups[${Config[type]}]=1
		done

		echo 'Config list:'
		printf '%s\n' "${ConfigList[@]}" | sort

		echo
		echo 'Config groups:'
		printf '%s\n' "${!ConfigGroups[@]}" | sort
		return 0
	fi

	declare ConfigName
	if [[ -v "$SelectedConfig""[@]" ]]; then
		# Full name
		ConfigName=$SelectedConfig
	else
		# Short name
		ConfigName=${ConfigList[$SelectedConfig]}
	fi

	# Run by type
	if [[ -z $ConfigName ]]; then
		declare -a FailedCfgs=()
		for ConfigName in "${ConfigList[@]}"; do
			declare -n Config=$ConfigName
			if [[ $SelectedConfig == "${Config[type]}" || $SelectedConfig == 'all' ]]; then
				run_config "${ConfigName}" || FailedCfgs+=("$ConfigName")
			fi
		done

		if (( ${#FailedCfgs} > 0 )); then
			echo 'Failed configs:'
			printf '%s\n' "${FailedCfgs[@]}"
			return 1
		else
			echo 'All configs passed!'
			return 0
		fi
	fi

	# Run the specific config
	run_config "${ConfigName}"
}

# Minimum supported configuration ----------------------------------------------

dockerfile_ubuntu_minimum() {
	# libglu deps are to get libsdl2-dev to install
	cat <<-'EOF'
		FROM ubuntu:14.04

		RUN apt-get update && \
		apt install g++ cmake mercurial pax-utils lintian \
			libsdl1.2-dev libsdl-net1.2-dev \
			libsdl2-dev libsdl2-net-dev \
			libflac-dev libogg-dev libvorbis-dev libopus-dev libopusfile-dev libmodplug-dev libfluidsynth-dev \
			zlib1g-dev libbz2-dev libgtk2.0-dev \
			libglu1-mesa-dev libglu1-mesa -y && \
		useradd -rm ecwolf && \
		echo "ecwolf ALL=(ALL) NOPASSWD: /usr/bin/make install" >> /etc/sudoers && \
		mkdir /home/ecwolf/results && \
		chown ecwolf:ecwolf /home/ecwolf/results && \
		ln -s /home/ecwolf/results /results

		USER ecwolf
	EOF
}

dockerfile_ubuntu_minimum_i386() {
	dockerfile_ubuntu_minimum | sed 's,FROM ,FROM i386/,'
}

# Performs a build of ECWolf. Extra CMake args can be passed as args.
build_ecwolf() {
	declare SrcDir=/mnt

	cd ~ || return

	# Check for previous invocation
	if [[ -d build ]]; then
		rm -rf build
	fi

	# Only matters on CMake 3.5+
	export CLICOLOR_FORCE=1

	mkdir build &&
	cd build &&
	cmake "$SrcDir" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr "$@" &&
	touch install_manifest.txt && # Prevent root from owning this file
	make -j "$(nproc)" || return
}
export -f build_ecwolf

test_build_ecwolf_cfg() {
	declare BuildCfg=$1
	shift

	build_ecwolf "$@" | tee "/results/buildlog-$BuildCfg.log"
	(( PIPESTATUS[0] == 0 )) || return 1

	cd ~/build &&
	sudo make install | tee "/results/installlog-$BuildCfg.log"
	(( PIPESTATUS[0] == 0 )) || return 1

	lddtree /usr/games/ecwolf | tee "/results/lddtree-$BuildCfg.txt"
}
export -f test_build_ecwolf_cfg

# Tests that supported configs compile and install
test_build_ecwolf() {
	declare -a FailedCfgs=()

	test_build_ecwolf_cfg SDL2 || FailedCfgs+=(SDL2)

	test_build_ecwolf_cfg SDL1 -DFORCE_SDL12=ON -DINTERNAL_SDL_MIXER=ON || FailedCfgs+=(SDL1)

	if (( ${#FailedCfgs} > 0 )); then
		echo 'Failed builds:'
		printf '%s\n' "${FailedCfgs[@]}"
		return 1
	else
		echo 'All passed!'
	fi
}
export -f test_build_ecwolf

# shellcheck disable=SC2034
declare -A ConfigUbuntuMinimum=(
	[dockerfile]=dockerfile_ubuntu_minimum
	[dockerimage]='ecwolf-ubuntu'
	[dockertag]=1
	[entrypoint]=test_build_ecwolf
	[prereq]=''
	[type]=test
)

# shellcheck disable=SC2034
declare -A ConfigUbuntuMinimumI386=(
	[dockerfile]=dockerfile_ubuntu_minimum_i386
	[dockerimage]='i386/ecwolf-ubuntu'
	[dockertag]=1
	[entrypoint]=test_build_ecwolf
	[prereq]=''
	[type]=test
)

# Ubuntu packaging -------------------------------------------------------------

dockerfile_ubuntu_package() {
	echo "FROM ${ConfigUbuntuMinimum[dockerimage]}:${ConfigUbuntuMinimum[dockertag]}"

	# Packaging requires CMake 3.11 or newer
	cat <<-'EOF'
		RUN cd ~ && \
		curl https://cmake.org/files/v3.12/cmake-3.12.1.tar.gz | tar xz && \
		cd cmake-3.12.1 && \
		./configure --parallel="$(nproc)" && \
		make -j "$(nproc)" && \
		sudo make install && \
		cd .. && \
		rm -rf cmake-3.12.1
	EOF
}

dockerfile_ubuntu_package_i386() {
	dockerfile_ubuntu_package | sed 's,FROM ,FROM i386/,'
}

package_ecwolf() {
	build_ecwolf || return

	make package &&
	lintian --suppress-tags embedded-library ecwolf_*.deb &&
	cp ecwolf_*.deb /results/
}
export -f package_ecwolf

# shellcheck disable=SC2034
declare -A ConfigUbuntuPackage=(
	[dockerfile]=dockerfile_ubuntu_package
	[dockerimage]='ecwolf-ubuntu-package'
	[dockertag]=1
	[entrypoint]=package_ecwolf
	[prereq]=ConfigUbuntuMinimum
	[type]=build
)

# shellcheck disable=SC2034
declare -A ConfigUbuntuPackageI386=(
	[dockerfile]=dockerfile_ubuntu_package_i386
	[dockerimage]='i386/ecwolf-ubuntu-package'
	[dockertag]=1
	[entrypoint]=package_ecwolf
	[prereq]=ConfigUbuntuMinimumI386
	[type]=build
)

# ------------------------------------------------------------------------------

declare -A ConfigList=(
	[ubuntumin]=ConfigUbuntuMinimum
	[ubuntumini386]=ConfigUbuntuMinimumI386
	[ubuntupkg]=ConfigUbuntuPackage
	[ubuntupkgi386]=ConfigUbuntuPackageI386
)

main "$@"; exit
