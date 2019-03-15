#!/bin/bash

# assumes nanocube is in the path

function test_create_no_header {

    # input_file_content: next 4 lines #
    cat <(cat <<EOF
2013/12/06 18:25:00,NARCOTICS,41.8789661034259,-87.72673345412568
2013/12/06 18:26:00,CRIMINAL DAMAGE,41.76836587673295,-87.68836274472295
2013/12/06 18:29:00,THEFT,41.810564946613454,-87.6753212816967
2013/12/06 18:30:00,THEFT,41.94189139041086,-87.65095594008946
EOF
) > tmp_input

    cat <(cat <<EOF
index_dimension('location',input(3,4),latlon(25));
index_dimension('type',input(2),categorical());
index_dimension('time', input(1), time('2009-01-01T00:00:00-05:00',3600));
measure_dimension('count',input(),f32);
EOF
) > tmp_map

    # how to create
    # change default sepearator from colon to pipe
    # use the default assumption of no header: note that map file uses 1-based column order
    nanocube create tmp_input tmp_map tmp.nanocube 2> /dev/null

    echo $?
}


function test_create_no_header_pipe {

    # input_file_content: next 4 lines #
    cat <(cat <<EOF
2013/12/06 18:25:00|NARCOTICS|41.8789661034259|-87.72673345412568
2013/12/06 18:26:00|CRIMINAL DAMAGE|41.76836587673295|-87.68836274472295
2013/12/06 18:29:00|THEFT|41.810564946613454|-87.6753212816967
2013/12/06 18:30:00|THEFT|41.94189139041086|-87.65095594008946
EOF
) > tmp_input

    cat <(cat <<EOF
index_dimension('location',input(3,4),latlon(25));
index_dimension('type',input(2),categorical());
index_dimension('time', input(1), time('2009-01-01T00:00:00-05:00',3600));
measure_dimension('count',input(),f32);
EOF
) > tmp_map

    # how to create
    # change default sepearator from colon to pipe
    # use the default assumption of no header: note that map file uses 1-based column order
    nanocube create tmp_input tmp_map tmp.nanocube -sep='|' 2> /dev/null

    echo $?
}

function test_create_no_header_tab {

    # input_file_content: next 4 lines #
    cat <(cat <<EOF
2013/12/06 18:25:00|NARCOTICS|41.8789661034259|-87.72673345412568
2013/12/06 18:26:00|CRIMINAL DAMAGE|41.76836587673295|-87.68836274472295
2013/12/06 18:29:00|THEFT|41.810564946613454|-87.6753212816967
2013/12/06 18:30:00|THEFT|41.94189139041086|-87.65095594008946
EOF
) | tr '|' '\t' > tmp_input

    cat <(cat <<EOF
index_dimension('location',input(3,4),latlon(25));
index_dimension('type',input(2),categorical());
index_dimension('time', input(1), time('2009-01-01T00:00:00-05:00',3600));
measure_dimension('count',input(),f32);
EOF
) > tmp_map

    # how to create
    # change default sepearator from colon to tab
    # use the default assumption of no header: note that map file uses 1-based column order
    nanocube create tmp_input tmp_map tmp.nanocube -sep='\t' 2> /dev/null

    echo $?
}

function test_create_no_header_001 {

    # input_file_content: next 4 lines #
    cat <(cat <<EOF
2013/12/06 18:25:00|NARCOTICS|41.8789661034259|-87.72673345412568
2013/12/06 18:26:00|CRIMINAL DAMAGE|41.76836587673295|-87.68836274472295
2013/12/06 18:29:00|THEFT|41.810564946613454|-87.6753212816967
2013/12/06 18:30:00|THEFT|41.94189139041086|-87.65095594008946
EOF
) | tr '|' '\001' > tmp_input

    cat <(cat <<EOF
index_dimension('location',input(3,4),latlon(25));
index_dimension('type',input(2),categorical());
index_dimension('time', input(1), time('2009-01-01T00:00:00-05:00',3600));
measure_dimension('count',input(),f32);
EOF
) > tmp_map

    # how to create
    # change default sepearator from colon to byte 0x01
    # use the default assumption of no header: note that map file uses 1-based column order
    nanocube create tmp_input tmp_map tmp.nanocube -sep='0x01' 2> /dev/null

    echo $?
}


function test_create_header_on_first_line {

    # input_file_content: next 4 lines #
    cat <(cat <<EOF
time,type,lat,lon
2013/12/06 18:25:00,NARCOTICS,41.8789661034259,-87.72673345412568
2013/12/06 18:26:00,CRIMINAL DAMAGE,41.76836587673295,-87.68836274472295
2013/12/06 18:29:00,THEFT,41.810564946613454,-87.6753212816967
2013/12/06 18:30:00,THEFT,41.94189139041086,-87.65095594008946
EOF
) > tmp_input

    cat <(cat <<EOF
index_dimension('location',input('lat','lon'),latlon(25));
index_dimension('type',input('type'),categorical());
index_dimension('time', input('time'), time('2009-01-01T00:00:00-05:00',3600));
measure_dimension('count',input(),f32);
EOF
) > tmp_map

    # how to create
    # change default sepearator from colon to pipe
    # use the default assumption of no header: note that map file uses 1-based column order
    nanocube create tmp_input tmp_map tmp.nanocube -header 2> /dev/null

    echo $?
}

function test_tile2d_range_query {

#
# 1 0 0 1
# 0 0 1 0
# 0 1 0 0
# 0 0 0 1
#

    # input_file_content: next 4 lines #
    cat <(cat <<EOF
x,y
0,3
1,1
2,2
3,3
3,0
EOF
) > tmp_input

    cat <(cat <<EOF
index_dimension('location',input('x','y'),xy(2));
measure_dimension('count',input(),u32);
EOF
) > tmp_map

    # how to create
    # change default sepearator from colon to pipe
    # use the default assumption of no header: note that map file uses 1-based column order
    nanocube create tmp_input tmp_map tmp.nanocube -header 2> /dev/null
    result_tile2d_range=$(nanocube query "{format('psv');q(x.b('location',tile2d_range(2,0,0,1,2)))}" x=tmp.nanocube 2> /dev/null | tail -n 1 | cut -f 1 -d'.')
    result_img2d_range=$(nanocube query "{format('psv');q(x.b('location',img2d_range(2,0,0,1,2)))}" x=tmp.nanocube 2> /dev/null | tail -n 1 | cut -f 1 -d'.')
    result="0"
    if [ "$result_tile2d_range" != "1" ]; then result="-1"; fi;
    if [ "$result_img2d_range" != "2" ]; then result="-1"; fi;
    echo $result
}




function test_drilldown_img2d_coords_psv {

#
# 1 0 0 1
# 0 0 1 0
# 0 1 0 0
# 0 0 0 1
#

    # input_file_content: next 4 lines #
    cat <(cat <<EOF
x,y
0,3
1,1
2,2
3,3
3,0
EOF
) > tmp_input

    cat <(cat <<EOF
index_dimension('location',input('x','y'),xy(2));
measure_dimension('count',input(),u32);
EOF
) > tmp_map

    # how to create
    # change default sepearator from colon to pipe
    # use the default assumption of no header: note that map file uses 1-based column order
    nanocube create tmp_input tmp_map tmp.nanocube -header 2> /dev/null
    nanocube query "{format('psv');q(x.b('location',dive(2),'img2'))}" x=tmp.nanocube 2> /dev/null > log
    nanocube query "{format('psv');q(x.b('location',dive(2),'tile2'))}" x=tmp.nanocube 2> /dev/null >> log
    echo 0
}

result_create_no_header=$(test_create_no_header)
echo $result_create_no_header | awk '{ if ($0 == "0") { printf "create_no_header: OK\n"; } else { printf "create_no_header: PROBLEM\n";} }'

result_create_no_header_pipe=$(test_create_no_header_pipe)
echo $result_create_no_header_pipe | awk '{ if ($0 == "0") { printf "create_no_header_pipe: OK\n"; } else { printf "create_no_header_pipe: PROBLEM\n";} }'

result_create_no_header_tab=$(test_create_no_header_tab)
echo $result_create_no_header_tab | awk '{ if ($0 == "0") { printf "create_no_header_tab: OK\n"; } else { printf "create_no_header_tab: PROBLEM\n";} }'

result_create_no_header_001=$(test_create_no_header_001)
echo $result_create_no_header_001 | awk '{ if ($0 == "0") { printf "create_no_header_001: OK\n"; } else { printf "create_no_header_001: PROBLEM\n";} }'

result_create_header_on_first_line=$(test_create_header_on_first_line)
echo $result_create_header_on_first_line | awk '{ if ($0 == "0") { printf "create_header_on_first_line: OK\n"; } else { printf "create_header_on_first_line: PROBLEM\n";} }'

result_tile2d_range_query=$(test_tile2d_range_query)
echo $result_tile2d_range_query | awk '{ if ($0 == "0") { printf "tile2d_range_query: OK\n"; } else { printf "tile2d_range_query: PROBLEM\n";} }'

result_drilldown_img2d_coords_psv=$(test_drilldown_img2d_coords_psv)
echo $result_drilldown_img2d_coords_psv | awk '{ if ($0 == "0") { printf "drilldown_img2d_coords_psv: OK\n"; } else { printf "drilldown_img2d_coords_psv: PROBLEM\n";} }'
