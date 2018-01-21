# Done
# Steps 1 .. 2
# TODO
# Steps 3 .. 6

if [[ $# -ne 3 ]]; then
	echo "Usage: $0 submissions.tar.gz solution.c keyword"
	exit 1
fi

submissions=$1
solution=$2
keyword=$3

copies_dir="Exact copies"
refed_dir="Referenced"
orig_dir="Original"
tmp_dir="_tmp_"

mkdir -p $tmp_dir
tar -xf $submissions -C $tmp_dir
mkdir -p "$copies_dir" "$refed_dir" "$orig_dir"

for file in $tmp_dir/*
do
	if cmp -s "$file" "$solution"; then
		echo "$file: same"
		mv "$file" "$copies_dir"
	else
		if grep -q "$keyword" "$file"; then
			echo "$file: has citation"
			mv "$file" "$refed_dir"
		else
			echo "$file: is original"
			mv "$file" "$orig_dir"
		fi
	fi
done
