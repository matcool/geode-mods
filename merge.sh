for mod_name in */; do
	if [ -f "$mod_name/mod.json" ]; then
		mod_name=${mod_name:0:-1}
		mod_id=$(grep -Po "\"id\": \".+?\"," $mod_name/mod.json | cut -d "\"" -f 4)
		echo "$mod_name - $mod_id"
		args=""
		if [ -f "built/$mod_name/Release/$mod_name.dll" ]; then
			mv "built/$mod_name/Release/$mod_name.dll" "built/$mod_name/Release/$mod_id.dll"
			args="$args --binary built/$mod_name/Release/$mod_id.dll"
		fi
		if [ -f "built/$mod_name/lib$mod_name.dylib" ]; then
			mv "built/$mod_name/lib$mod_name.dylib" "built/$mod_name/$mod_id.dylib"
			args="$args --binary built/$mod_name/$mod_id.dylib"
		fi
		./geode.exe package new . $args --output $mod_name.geode
	fi
done