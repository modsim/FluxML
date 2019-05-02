<?xml version="1.0"?>
<measurement xmlns="http://www.uni-siegen.de/fb11/simtec/13cflux/mm"
             xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
             xsi:schemaLocation="http://www.uni-siegen.de/fb11/simtec/13cflux/mm http://www.uni-siegen.de/fb11/simtec/13cflux/mm.xsd">
  <mlabel>
    <!-- alle Unterelemente optional: -->
    <date>2007-08-07 11:11:11</date>
    <version>1.1</version>
    <comment>Messmodell zum Spiral-Netzwerk</comment>
    <fluxunit>gigamol/year</fluxunit>
    <poolsizeunit>gigamol</poolsizeunit>
  </mlabel>
  <model stationary="false">
    <labelingmeasurement>
      <MSgroup id="MS_Messung" spec="C[1-3]#M0,1,2" times="0.1 0.2 0.3"/>
    </labelingmeasurement>
  </model>
  <data id="versuch_1">
    <dlabel>
      <start>2007-08-07 11:11:11</start>
      <finish>2007-08-07 11:11:12</finish>
      <people>Michael</people>
      <strain>Bacillus Spiralus Vulgaris</strain>
      <comment>Messwerte für MS-Messung</comment>
    </dlabel>
    <!-- MS-Messung ms_1: -->
    <datum id="MS_Messung" time="0.1" stddev="0.1" weight="0">4.321</datum>
    <datum id="MS_Messung" time="0.1" stddev="0.1" weight="1">1.2</datum>
    <datum id="MS_Messung" time="0.1" stddev="0.1" weight="2">1.23</datum>
    <datum id="MS_Messung" time="0.2" stddev="0.1" weight="0">5.301</datum>
    <datum id="MS_Messung" time="0.2" stddev="0.1" weight="1">2.219</datum>
    <datum id="MS_Messung" time="0.2" stddev="0.1" weight="2">1.27</datum>
    <datum id="MS_Messung" time="0.3" stddev="0.1" weight="0">3.1</datum>
    <datum id="MS_Messung" time="0.3" stddev="0.1" weight="1">3.88</datum>
    <datum id="MS_Messung" time="0.3" stddev="0.1" weight="2">3.36</datum>
  </data>
</measurement>

