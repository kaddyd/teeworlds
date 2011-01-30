#/bin/sh

if [ -z "`which gconftool-2`" ]; then
	echo "Gnome not found (or missing gconftool-2)"
else
	gconftool-2 -s /desktop/gnome/url-handlers/teeworlds/command "\"`pwd`/teeworlds\" \"connect %s\"" --type String
	gconftool-2 -s /desktop/gnome/url-handlers/teeworlds/enabled --type Boolean true
	echo "Gnome protocol handler registered"
fi

if [ -z "$KDEDIR" ]; then
	if [ -d ~/.kde ]; then
		mkdir -p ~/.kde/share/services/
		echo "[Protocol]" > ~/.kde/share/services/teeworlds.protocol
		echo "exec=\"`pwd`/teeworlds\" \"connect %u\"" >> ~/.kde/share/services/teeworlds.protocol
		echo "protocol=teeworlds" >> ~/.kde/share/services/teeworlds.protocol
		echo "input=none" >> ~/.kde/share/services/teeworlds.protocol
		echo "output=none" >> ~/.kde/share/services/teeworlds.protocol
		echo "helper=true" >> ~/.kde/share/services/teeworlds.protocol
		echo "listing=" >> ~/.kde/share/services/teeworlds.protocol
		echo "reading=false" >> ~/.kde/share/services/teeworlds.protocol
		echo "writing=false" >> ~/.kde/share/services/teeworlds.protocol
		echo "makedir=false" >> ~/.kde/share/services/teeworlds.protocol
		echo "deleting=false" >> ~/.kde/share/services/teeworlds.protocol
		echo "KDE protocol handler registered (missing \$KDEDIR, used ~/.kde instead)"
	else
		echo "KDE not found (or missing \$KDEDIR and ~/.kde)"
	fi
else
	mkdir -p $KDEDIR/share/services/
	echo "[Protocol]" > $KDEDIR/share/services/teeworlds.protocol
	echo "exec=\"`pwd`/teeworlds\" \"connect %u\"" >> $KDEDIR/share/services/teeworlds.protocol
	echo "protocol=teeworlds" >> $KDEDIR/share/services/teeworlds.protocol
	echo "input=none" >> $KDEDIR/share/services/teeworlds.protocol
	echo "output=none" >> $KDEDIR/share/services/teeworlds.protocol
	echo "helper=true" >> $KDEDIR/share/services/teeworlds.protocol
	echo "listing=" >> $KDEDIR/share/services/teeworlds.protocol
	echo "reading=false" >> $KDEDIR/share/services/teeworlds.protocol
	echo "writing=false" >> $KDEDIR/share/services/teeworlds.protocol
	echo "makedir=false" >> $KDEDIR/share/services/teeworlds.protocol
	echo "deleting=false" >> $KDEDIR/share/services/teeworlds.protocol
	echo "KDE protocol handler registered"
fi
