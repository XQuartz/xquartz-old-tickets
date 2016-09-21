#!/bin/bash
# (c) 1999-2004 Gentoo Foundation (portions from Gentoo's /etc/init.d/xfs)
# Portions copyright (c) 2008 Apple Inc.  All rights reserved.
# Distributed under the terms of the GNU General Public License, v2

X11DIR=/usr/X11
X11FONTDIR=${X11DIR}/lib/X11/fonts

# System fonts or user fonts?
system=0
force=0

# Return 0 on change, or 1 on no change, or if dir do not exist
check_changed() {
	local x=""
	local fontlist=""
	
	# If the dir do not exist, e
	if [[ ! -d $1 ]]; then
		return 1
	fi

	if [[ ${force} == 1 ]] ; then
		return 0
	fi

	# Create a list of all non known config files in the font dir
	fontlist="$(find $1/ -maxdepth 1 -type f | \
	            awk '$0 !~ /fonts\..*$|^.*\.dir$|XftCache/ {print}')"
	
	if [[ ! -f $1/fonts.list ]] ; then
		if [[ -n "${fontlist}" ]] ; then
			# No list file exist, so create it and return 0 to add
			# this font dir as a candidate for updating...
			echo "${fontlist}" > "$1"/fonts.list

			return 0
		fi
	else
		local retval=1

		# All the fonts were removed, so cleanup
		if [[ -z "${fontlist}" ]] ; then
			for x in "$1"/fonts.* "$1"/encodings.dir "$1"/XftCache ; do
				if [[ -f ${x} ]] ; then
					rm -f "${x}"
				fi
			done

			return 1
		fi

		# Check if we have fonts.dir
		if [[ ! -f $1/fonts.dir ]]; then
			return 0
		fi

		# Check that no files was added or removed....
		if [[ "$(cat "$1"/fonts.list)" != "${fontlist}" ]] ; then
			retval=0
		fi

		# Check that no files was updated....
		if [[ "${retval}" -ne 0 ]] ; then
			local changed_list=""

			changed_list="$(find "$1"/ -type f -cnewer "$1"/fonts.dir | \
			                awk '$0 !~ /fonts\.(list|cache-1)$|XftCache/ {print}')"

			if [[ -n "${changed_list}" ]] ; then
				retval=0
			fi
		fi

		# OK, something changed, so recreate fonts.list and add as candidate
		# for updating...
		if [[ "${retval}" -eq 0 ]] ; then
			echo "${fontlist}" > "$1"/fonts.list

			return 0
		fi
	fi

	return 1
}

get_fontdir_list() {
	local d
	if [[ $system == 1 ]] ; then
		find {/System/,/}Library/Fonts -type d

		for d in "${X11FONTDIR}"/* ; do
			case ${d#${X11FONTDIR}/} in
				conf*|encodings*) ;;
				*) find "$d" -type d ;;
			esac
		done
	else 
		for d in "${HOME}"/{.fonts,Library/Fonts} ; do
			if [[ -d $d ]] ; then
				find "$d" -type d
			fi
		done
	fi
}

# This is the main beast for setting up the font dirs
setup_font_dirs() {
	local x=""
	local pending_fontdirs=""
	local changed="no"

	umask 022

	# Generate the encodings.dir ...
	if [[ $system == 1 ]] ; then
		${X11DIR}/bin/mkfontdir -n \
			-e ${X11FONTDIR}/encodings \
			-e ${X11FONTDIR}/encodings/large \
			-- ${X11FONTDIR}/encodings
	fi

	if [[ $system == 1 ]] ; then
		echo "font_cache.sh: Scanning system font directories to generate X11 font caches"
	else
		echo "font_cache.sh: Scanning user font directories to generate X11 font caches"
	fi

	OIFS=$IFS
	IFS='
'
	for x in $(get_fontdir_list) ; do
		if test -d "${x}" && check_changed "${x}" ; then
			if [[ -z "${pending_fontdirs}" ]] ; then
				pending_fontdirs="${x}"
			else
				pending_fontdirs="${pending_fontdirs}${IFS}${x}"
			fi
		fi
	done

	if [[ -n "${pending_fontdirs}" ]] ; then
		echo "font_cache.sh: Indexing font directories"
		for x in ${pending_fontdirs} ; do
			echo "font_cache.sh:    ${x}"

			# Only generate .scale files if truetype, opentype or type1
			# fonts are present ...

			# First truetype (ttf,ttc)
			# NOTE: ttmkfdir does NOT work on type1 fonts (#53753)
			# Also, there is no way to regenerate Speedo/CID fonts.scale
			# <spyderous@gentoo.org> 2 August 2004
			if [ "${x/encodings}" = "${x}" -a \
			     -n "$(find ${x} -iname '*.tt[cf]' -print)" ]
			then

				if [ -x "${X11DIR}/bin/ttmkfdir" ] ; then
					${X11DIR}/bin/ttmkfdir -x 2 \
						-e ${X11FONTDIR}/encodings/encodings.dir \
						-o ${x}/fonts.scale -d ${x} > /dev/null
					# ttmkfdir fails on some stuff, so try mkfontscale if it does
					local ttmkfdir_return=$?
				else
					ttmkfdir_return=1
				fi

				if [ ${ttmkfdir_return} -ne 0 ]
				then
					${X11DIR}/bin/mkfontscale \
						-a ${X11FONTDIR}/encodings/encodings.dir \
						-- ${x}
				fi

			# Next type1 and opentype (pfa,pfb,otf,otc)
			elif [ "${x/encodings}" = "${x}" -a \
				-n "$(find ${x} -iname '*.[po][ft][abcf]' -print)" ]
			then
				${X11DIR}/bin/mkfontscale \
					-a ${X11FONTDIR}/encodings/encodings.dir \
					-- ${x}
			fi
          
		  	# Now generate fonts.dir files ...
			if [[ "${x/encodings}" = "${x}" ]] ; then
				${X11DIR}/bin/mkfontdir \
					-e ${X11FONTDIR}/encodings \
					-e ${X11FONTDIR}/encodings/large \
					-- ${x} > /dev/null
			fi

			if [ "${x/encodings}" = "${x}" -a -x ${X11DIR}/bin/xftcache ] && \
			   [ -n "$(find ${x} -iname '*.[otps][pft][cfad]' -print)" ]
			then
				# xftcache is broken, but run it anyhow ...
				${X11DIR}/bin/xftcache ${x} &> /dev/null
			fi

			changed="yes"
		done
	fi
	IFS=$OIFS

	# While we at it, update fontconfig's cache as well
	echo "font_cache.sh: Updating FC cache"
	if [[ $system == 1 ]] ; then
		HOME="$(echo ~root)" ${X11DIR}/bin/fc-cache $([[ $force == 1 ]] && echo "-f -r")
	else
		${X11DIR}/bin/fc-cache $([[ $force == 1 ]] && echo "-f -r")
	fi
	echo "font_cache.sh: Done"
}

# TODO: Make this more sexy
if [[ $1 == "-s" || $2 == "-s" ]] ; then
	system=1
fi

if [[ $1 == "-f" || $2 == "-f" ]] ; then
	force=1
fi

setup_font_dirs "${@}"
