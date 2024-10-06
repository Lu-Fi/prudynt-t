#!/bin/bash
HOME_DIR=$HOME
GIT_REPO=$(git remote get-url origin | sed -E 's#.*/([^/]+)\.git#\1#')
GIT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
GIT_HASH=$(git rev-parse --short HEAD)
GIT_TIME=$(git show -s --format=%ci)
BACKTITLE="$GIT_REPO - $GIT_BRANCH+$GIT_HASH, $GIT_TIME"
UI=dialog
DIALOG_COMMON=($UI --keep-tite --colors --backtitle "$BACKTITLE" --cancel-label "Exit" --title "$GIT_REPO Buildroot Compile")

show_help() {
	cat << EOF
Buildroot Prudynt Build
Usage: ./build.sh [-d] [-c] <profile_name>
Example: ./build.sh wyze_cp2

-d	Enable debug mode (-O0 -g)
-c    Disable ccache

Note: Set the DEST_DIR environment variable to specify the directory where the Prudynt binary will be copied after the build.
EOF
	exit 0
}

require_dialog() {
	if ! command -v dialog &> /dev/null; then
		echo -e "'dialog' is required for the interactive menu. Please install it.\n"
		show_help
		exit 1
	fi
}

use_ccache() {
	CCACHE_BIN="$HOME_DIR/output/$PROFILE_NAME/host/bin/ccache"
	if [ ! -f "$CCACHE_BIN" ]; then
		echo -e "'ccache' is required but not found in the selected profile: $CCACHE_BIN.\nPlease complete a full build first."
		exit 1
	fi
}

select_profile() {
	local profiles=($(ls -d "$HOME_DIR/output"/*/ 2>/dev/null | xargs -r -n 1 basename))

	if [ ${#profiles[@]} -eq 0 ]; then
		echo "No profiles found in $HOME_DIR/output. Please create a profile first."
		exit 1
	fi

	local profile_menu=()
	for i in "${!profiles[@]}"; do
		profile_menu+=($((i + 1)) "${profiles[i]}")
	done

	[ $DEBUG_MODE -eq 1 ] && DEBUG_MODE_TXT="\n\Zb\Z1DEBUG\Zn Mode enabled!\n"
	[ $USE_CCACHE -eq 0 ] && USE_CCACHE_TXT="\n\Zb\Z1ccache\Zn disabled!\n"
	[ -n "$DEST_DIR" ] && DEST_DIR_TXT="\nCompiled binary will be copied to: \Zb\Z1$DEST_DIR\Zn"

	while true; do
		local selected_tag=$("${DIALOG_COMMON[@]}" --help-button --menu "Please select a buildroot profile:$USE_CCACHE_TXT$DEBUG_MODE_TXT$DEST_DIR_TXT" 15 50 10 "${profile_menu[@]}" 2>&1 >/dev/tty)

		if [[ "$selected_tag" =~ HELP ]]; then
			"${DIALOG_COMMON[@]}" --msgbox "Buildroot Prudynt Build
Usage: ./build.sh [-d] [-c] <profile_name>
Example: ./build.sh wyze_cp2

-d    Enable debug mode (-O0 -g)
-c    Disable ccache

Note: Set the DEST_DIR environment variable to specify the directory where the Prudynt binary will be copied after the build." 14 60
			continue
		fi

		if [ -z "$selected_tag" ]; then
			exit 1
		fi

		PROFILE_NAME="${profiles[$((selected_tag - 1))]}"
		break
	done
}

validate_profile() {
	if [ ! -f "$HOME_DIR/output/$PROFILE_NAME/.config" ]; then
		echo "Profile config does not exist: $PROFILE_NAME"
		exit 1
	fi

	if [ ! -d "$HOME_DIR/output/$PROFILE_NAME/target" ]; then
		echo -e "Target directory does not exist for $PROFILE_NAME.\nPlease complete a full build first for the selected profile."
		exit 1
	fi
}

# Main logic
DEBUG_MODE=0
USE_CCACHE=1
OPT_CMD="-Os"

while [[ $# -gt 0 ]]; do
	case "$1" in
		-h)
			show_help
			;;
		-d)
			DEBUG_MODE=1
			OPT_CMD="-O0 -g"
			shift
			;;
		-c)
			USE_CCACHE=0
			shift
			;;
		*)
			PROFILE_NAME=$1
			shift
			;;
	esac
done

if [ -z "$PROFILE_NAME" ]; then
	require_dialog
	[ ! -d "$HOME_DIR/output" ] && echo -e "Output directory does not exist in $HOME.\nPlease complete a full build first." && exit 1
	select_profile
fi

validate_profile

if [ $USE_CCACHE -eq 1 ]; then
	use_ccache
fi

CONFIG_FILE="$HOME_DIR/output/$PROFILE_NAME/.config"
PLAT=$(sed -n 's/BR2_SOC_FAMILY_INGENIC_\([^=]*\)=.*/\1/p' "$CONFIG_FILE")

make distclean
time /usr/bin/make V=1 -j$(( $(nproc) + 1 )) \
	ARCH= \
	CROSS_COMPILE="$CCACHE_BIN $HOME_DIR/output/$PROFILE_NAME/per-package/prudynt-t/host/bin/mipsel-linux-" \
	CFLAGS="-DPLATFORM_$PLAT $OPT_CMD -DALLOW_RTSP_SERVER_PORT_REUSE=1 -DNO_OPENSSL=1 \
	-I$HOME_DIR/output/$PROFILE_NAME/per-package/prudynt-t/host/mipsel-buildroot-linux-musl/sysroot/usr/include \
	-I$HOME_DIR/output/$PROFILE_NAME/per-package/prudynt-t/host/mipsel-buildroot-linux-musl/sysroot/usr/include/liveMedia \
	-I$HOME_DIR/output/$PROFILE_NAME/per-package/prudynt-t/host/mipsel-buildroot-linux-musl/sysroot/usr/include/groupsock \
	-I$HOME_DIR/output/$PROFILE_NAME/per-package/prudynt-t/host/mipsel-buildroot-linux-musl/sysroot/usr/include/UsageEnvironment \
	-I$HOME_DIR/output/$PROFILE_NAME/per-package/prudynt-t/host/mipsel-buildroot-linux-musl/sysroot/usr/include/BasicUsageEnvironment" \
	LDFLAGS="-z max-page-size=0x1000 -L$HOME_DIR/output/$PROFILE_NAME/per-package/prudynt-t/host/mipsel-buildroot-linux-musl/sysroot/usr/lib \
	-L$HOME_DIR/output/$PROFILE_NAME/per-package/prudynt-t/target/usr/lib" \
	-C "$PWD" all

if [ -n "$DEST_DIR" ]; then
	cp bin/prudynt "$DEST_DIR"
	echo "Binary copied to $DEST_DIR"
fi
