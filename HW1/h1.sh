# Done
# Steps 1 .. 4
# TODO
# Steps 5 .. 6

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
report_file="report.txt"

copies_cnt=0
refed_cnt=0
orig_cnt=0

mkdir -p $tmp_dir
tar -xf $submissions -C $tmp_dir
mkdir -p "$copies_dir" "$refed_dir" "$orig_dir"

for file in $tmp_dir/*
do
	if cmp -s "$file" "$solution"; then
		# echo "$file: same"
		mv "$file" "$copies_dir"
		let copies_cnt+=1
	else
		if grep -q "$keyword" "$file"; then
			# echo "$file: has citation"
			mv "$file" "$refed_dir"
			let refed_cnt+=1
		else
			# echo "$file: is original"
			mv "$file" "$orig_dir"
			let orig_cnt+=1
		fi
	fi
done

# orig_ratio=$(echo "scale=2;$orig_cnt/($orig_cnt+$refed_cnt+$copies_cnt)" | bc -l)
orig_ratio=$(( 100 * $orig_cnt / ($orig_cnt + $refed_cnt + $copies_cnt) ))
echo "Exact copies  $copies_cnt"		>  $report_file
echo "Referenced    $refed_cnt"			>> $report_file
echo "Original      $orig_cnt"			>> $report_file
echo "Ratio         ${orig_ratio}%"	>> $report_file
