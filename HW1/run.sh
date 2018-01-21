work_dir="__test__"
tar_file="_run_.tar.gz"
script_file="h1.sh"
script_args="sub.tar.gz sol.c stackoverflow.com"
report_file="report.txt"

rm -rf $work_dir
mkdir -p $work_dir
tar -xzf $tar_file -C $work_dir
cp $script_file ${work_dir}/${script_file}
cd $work_dir
./$script_file $script_args

cat "$report_file"
