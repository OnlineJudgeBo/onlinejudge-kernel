#!/bin/bash

service judged stop
cd $PWD/judge
make
chmod +x judged
cp -f  judged /usr/bin

cd ../judge_client
make
chmod +x judge_client
cp -f judge_client /usr/bin

cd ..
cd sim/sim_3_01/
make fresh
make exes
chmod +x sim*

cp -f sim_c.exe /usr/bin/sim_c
cp -f sim_java.exe /usr/bin/sim_java
cp -f sim_pasc.exe /usr/bin/sim_pas
cp -f sim_text.exe /usr/bin/sim_text
cd ..

cp sim.sh /usr/bin
chmod +x /usr/bin/sim.sh

rm /usr/bin/sim_cc /usr/bin/sim_rb /usr/bin/sim_sh
ln -sf /usr/bin/sim_c /usr/bin/sim_cc
ln -sf /usr/bin/sim_text /usr/bin/sim_rb
ln -sf /usr/bin/sim_text /usr/bin/sim_sh
cd ..

#########

cp judged /etc/init.d/judged
chmod +x  /etc/init.d/judged

ln -sf /etc/init.d/judged /etc/rc3.d/S93judged
ln -sf /etc/init.d/judged /etc/rc2.d/S93judged

systemctl daemon-reload
service judged start
