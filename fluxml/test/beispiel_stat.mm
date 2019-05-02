<?xml version="1.0" encoding="utf-8"?>
<measurement xmlns="http://www.uni-siegen.de/fb11/simtec/13cflux/mm" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:schemaLocation="http://www.uni-siegen.de/fb11/simtec/13cflux/mm http://www.uni-siegen.de/fb11/simtec/13cflux/mm.xsd">
  <mlabel>
    <!-- alle Unterelemente optional: -->
    <date>2007-05-15 11:11:11</date>
    <version>1.0</version>
    <comment>kein Kommentar</comment>
    <fluxunit>gigamol/year</fluxunit>
    <poolsizeunit>gigamol</poolsizeunit>
  </mlabel>
  <model stationary="true">
    <labelingmeasurement>
      <MSgroup id="ms_1" spec="Blubb[1-3,5,6]#M0,1,2"/>
      <MSMSgroup id="msms_1" spec="Bla[1-3:2-3]#M(3,0),(3,1),(3,2)" scale="one"/>
      <NMR1Hgroup id="nmr1h_1" spec="Frupp#P1,2"/>
      <NMR13Cgroup id="nmr13c_1" spec="Flapp#S1,T1,3"/>
      <group id="gen_1">
<!-- mehrere Zeilen mit Formeln. Jede Zeile nur Ausdrücke der Dimension 1
     Alle(!) Typen erlaubt -->
        <textual>
          0.7*Frup#xx01x - 1/3*Bla#111x + 1.414213562e-7*Gamma#xxx1 - 3.1415;
	  Gonzo#1xx - 3*Kermit#xxx * MissPiggy#xxx1
	</textual>
      </group>
    </labelingmeasurement>
    
    <fluxmeasurement>
      <!-- times ist hier vorerst immer optional?! -->
      <netflux id="fn_1" fluxes="upt1 blupt2"/>
      <xchflux id="fx_1" fluxes="upt1 blupt2"/>
    </fluxmeasurement>

    <poolmeasurement>
      <!-- times ist hier vorerst immer optional?! -->
      <pgroup id="p_1" pools="Gluc PEP"/>
    </poolmeasurement>
    
    <fluxratios>
      <!-- times ist hier vorerst immer optional?! -->
      <netratio id="frn_1">(bla/flupp)=(bläh/blubb)</netratio>
      <xchratio id="frx_1">blöd/upt=upt/blubb</xchratio>
    </fluxratios>
  </model>
  <data id="versuch_1">
    <dlabel>
      <start>2007-05-15 11:11:11</start>
      <finish>2007-05-15 11:11:12</finish>
      <people>Michael</people>
      <strain>Drosophila coli K42</strain>
      <comment>blabla bla flupp frupp</comment>
    </dlabel>	
    <!-- MS-Messung ms_1: -->
    <datum id="ms_1" stddev="0.1" weight="0">4.321</datum>
    <datum id="ms_1" stddev="0.2" weight="1">1.234</datum>
    <datum id="ms_1" stddev="0.3" weight="2">1.234</datum>

    <!-- MSMS-Messung msms_1: -->
    <datum id="msms_1" stddev="0.01" weight="3,0">0.001</datum>
    <datum id="msms_1" stddev="0.11" weight="3,1">0.002</datum>
    <datum id="msms_1" stddev="0.21" weight="3,2">0.003</datum>

    <!-- 1H-NMR-Messung nmr1h_1: -->
    <datum id="nmr1h_1" stddev="0.31" pos="1">0.11</datum>
    <datum id="nmr1h_1" stddev="0.41" pos="2">0.12</datum>

    <!-- 13C-NMR-Messung nmr13c_1: -->
    <datum id="nmr13c_1" stddev="0.71" pos="1" type="S">0.15</datum>
    <datum id="nmr13c_1" stddev="0.81" pos="1" type="T">0.16</datum>
    <datum id="nmr13c_1" stddev="0.91" pos="3" type="T">0.17</datum>

    <!-- generische Messung gen_1 -->
    <datum id="gen_1" row="1" stddev="0.02">0.111111</datum>
    <datum id="gen_1" row="2" stddev="0.12">0.222222</datum>

    <!-- Flussmessungen fn_1, fx_1: -->
    <datum id="fn_1" stddev="0.22">9.81</datum>
    <datum id="fx_1" stddev="0.32">3.14</datum>

    <!-- Poolmessung p_1: -->
    <datum id="p_1" stddev="0.42">1.41</datum>
    <datum id="p_1" stddev="0.52">2.82</datum>
  </data>
</measurement>

