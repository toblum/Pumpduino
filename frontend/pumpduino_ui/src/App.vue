<template>
  <v-app>
    <v-toolbar app>
      <v-toolbar-title class="headline text-uppercase">
        <span>Pumpduino UI v2</span>
      </v-toolbar-title>
      <v-spacer></v-spacer>
      <v-btn
        flat
        href="http://diskstation.fritz.box:1880/#flow/ea0f4d3d.2dd7a"
        target="_blank"
      >
        <span class="mr-2">Node RED</span>
      </v-btn>
    </v-toolbar>

    <v-content>
      <v-container>
        <v-layout>
          <v-flex xs12>
            <v-toolbar dense>
              <v-flex xs3>
                <v-btn :color="(is_auto) ? 'primary' : ''"  :flat="!is_auto" block @click="onWorkingModeClick('setauto')">
                  Auto
                </v-btn>
              </v-flex>
              <v-flex xs3>
                <v-btn :color="(is_on) ? 'primary' : ''"  :flat="!is_on" block @click="onWorkingModeClick('seton')">
                  On
                </v-btn>
              </v-flex>
              <v-flex xs3>
                <v-btn :color="(is_off) ? 'primary' : ''"  :flat="!is_off" block @click="onWorkingModeClick('setoff')">
                  Off
                </v-btn>
              </v-flex>
              <v-flex xs3>
                <v-btn :color="(is_once) ? 'primary' : ''"  :flat="!is_once" block @click="onWorkingModeClick('setonce')">
                  Once
                </v-btn>
              </v-flex>
            </v-toolbar>
          </v-flex>
        </v-layout>
      </v-container>

      <v-container grid-list-md>
        <v-layout row wrap>
          <v-flex xs12 sm6>
            <v-card>
              <v-list dense>
                <v-list-tile>
                  <v-list-tile-content><span class="font-weight-medium">Vorlauf:</span></v-list-tile-content>
                  <v-list-tile-content class="align-end">{{temp_vorlauf}}°</v-list-tile-content>
                </v-list-tile>
                <v-list-tile>
                  <v-list-tile-content><span class="font-weight-medium">Rücklauf:</span></v-list-tile-content>
                  <v-list-tile-content class="align-end">{{temp_ruecklauf}}°</v-list-tile-content>
                </v-list-tile>
                <v-list-tile>
                  <v-list-tile-content><span class="font-weight-medium">Raum:</span></v-list-tile-content>
                  <v-list-tile-content class="align-end">{{temp_raum}}°</v-list-tile-content>
                </v-list-tile>
              </v-list>
            </v-card>
          </v-flex>
          <v-flex xs12 sm6>
            <v-card>
              <v-list dense>
                <v-list-tile>
                  <v-list-tile-content><span class="font-weight-medium">Umwälzpumpe:</span></v-list-tile-content>
                  <v-list-tile-content class="align-end">{{(switch1_state) ? "On" : "Off"}}</v-list-tile-content>
                </v-list-tile>
                <v-list-tile>
                  <v-list-tile-content><span class="font-weight-medium">Steuerungsstatus:</span></v-list-tile-content>
                  <v-list-tile-content class="align-end">{{steeringModeString}}</v-list-tile-content>
                </v-list-tile>
                <v-list-tile>
                  <v-list-tile-content><span class="font-weight-medium">Rechenmodus:</span></v-list-tile-content>
                  <v-list-tile-content class="align-end">{{(calc_mode == "1") ? "Temperaturunterschied" : "Zieltemperatur"}}</v-list-tile-content>
                </v-list-tile>
              </v-list>
            </v-card>
          </v-flex>
        </v-layout>
      </v-container>

      <v-container>
        <v-expansion-panel>
          <v-expansion-panel-content>
            <div slot="header"><span class="font-weight-medium">Einstellungen</span></div>
            <v-container>
              <v-layout row wrap>

                <v-flex xs12 lg6>
                  <v-list dense>
                    <v-list-tile>
                      <v-list-tile-content><span class="font-weight-medium">Minimalpumpdauer:</span> {{formatSeconds(MINIMALPUMPDAUER)}}</v-list-tile-content>
                      <v-list-tile-content class="align-end">
                        <v-flex xs12><v-text-field single-line small v-model="MINIMALPUMPDAUER"></v-text-field></v-flex>
                      </v-list-tile-content>
                      <v-list-tile-content class="align-end">
                        <v-btn @click="saveSetting('setminp', MINIMALPUMPDAUER)"><v-icon dark>save</v-icon></v-btn>
                      </v-list-tile-content>
                    </v-list-tile>

                    <v-list-tile>
                      <v-list-tile-content><span class="font-weight-medium">Maximalpumpdauer:</span> {{formatSeconds(MAXIMALPUMPDAUER)}}</v-list-tile-content>
                      <v-list-tile-content class="align-end">
                        <v-flex xs12><v-text-field single-line small v-model="MAXIMALPUMPDAUER"></v-text-field></v-flex>
                      </v-list-tile-content>
                      <v-list-tile-content class="align-end">
                        <v-btn @click="saveSetting('setmaxp', MAXIMALPUMPDAUER)"><v-icon dark>save</v-icon></v-btn>
                      </v-list-tile-content>
                    </v-list-tile>

                    <v-list-tile>
                      <v-list-tile-content><span class="font-weight-medium">Pumpenschutzzeit:</span> {{formatSeconds(PUMPENSCHUTZZEIT)}}</v-list-tile-content>
                      <v-list-tile-content class="align-end">
                        <v-flex xs12><v-text-field single-line small v-model="PUMPENSCHUTZZEIT"></v-text-field></v-flex>
                      </v-list-tile-content>
                      <v-list-tile-content class="align-end">
                        <v-btn @click="saveSetting('setpumps', PUMPENSCHUTZZEIT)"><v-icon dark>save</v-icon></v-btn>
                      </v-list-tile-content>
                    </v-list-tile>
                  </v-list>
                </v-flex>


                <v-flex xs12 lg6>
                  <v-list dense>
                    <v-list-tile>
                      <v-list-tile-content><span class="font-weight-medium">Zieltemperatur:</span> {{RUECKTARGETTEMP.toFixed(2)}} °C</v-list-tile-content>
                      <v-list-tile-content class="align-end">
                        <v-flex xs12><v-text-field single-line small v-model="RUECKTARGETTEMP"></v-text-field></v-flex>
                      </v-list-tile-content>
                      <v-list-tile-content class="align-end">
                        <v-btn @click="saveSetting('settt', RUECKTARGETTEMP)"><v-icon dark>save</v-icon></v-btn>
                      </v-list-tile-content>
                    </v-list-tile>

                    <v-list-tile>
                      <v-list-tile-content><span class="font-weight-medium">Zieltemperaturdifferenz:</span> {{TARGETDIFFERENCE.toFixed(2)}} °C</v-list-tile-content>
                      <v-list-tile-content class="align-end">
                        <v-flex xs12><v-text-field single-line small v-model="TARGETDIFFERENCE"></v-text-field></v-flex>
                      </v-list-tile-content>
                      <v-list-tile-content class="align-end">
                        <v-btn @click="saveSetting('setdiff', TARGETDIFFERENCE)"><v-icon dark>save</v-icon></v-btn>
                      </v-list-tile-content>
                    </v-list-tile>

                    <v-list-tile>
                      <v-list-tile-content><span class="font-weight-medium">Rechenmodus:</span> <!-- {{(calc_mode == "1") ? "Temperaturunterschied" : "Zieltemperatur"}} --></v-list-tile-content>
                      <v-list-tile-content class="align-end">
                        <v-flex xs8>
                          <v-select :items="calc_items" v-model="calc_mode"></v-select>
                        </v-flex>
                      </v-list-tile-content>
                      <v-list-tile-content class="align-end">
                        <v-btn @click="saveSetting('setcalc', calc_mode)"><v-icon dark>save</v-icon></v-btn>
                      </v-list-tile-content>
                    </v-list-tile>

                  </v-list>
                </v-flex>
              </v-layout>
            </v-container>
          </v-expansion-panel-content>
        </v-expansion-panel>
      </v-container>

    </v-content>
  </v-app>
</template>


<script>
/*eslint no-console: "off"*/

var host = window.location.hostname;
if (host === "localhost") {
  host = "192.168.0.54";
}

export default {
  name: 'PumpduinoUI',

  components: {
    // HelloWorld
  },

  data () {
    return {
      temp_vorlauf: 0,
      temp_ruecklauf: 0,
      temp_raum: 0,
      switch1_state: false,
      steering_mode: 0,
      calc_mode: 0,
      working_mode: 0,

      MINIMALPUMPDAUER: 0,
      MAXIMALPUMPDAUER: 0,
      PUMPENSCHUTZZEIT: 0,
      TARGETDIFFERENCE: 0,
      RUECKTARGETTEMP: 0,

      calc_items: [{value: 0, text: "Zieltemperatur"}, {value: 1, text: "Temperaturunterschied"}]
    }
  },

  computed: {
    is_off: function () {
      return (this.working_mode == "0");
    },
    is_on: function () {
      return (this.working_mode == "1");
    },
    is_auto: function () {
      return (this.working_mode == "2");
    },
    is_once: function () {
      return (this.working_mode == "3");
    },
    steeringModeString: function () {
      let res = "";
      if (this.steering_mode == "0") {
        res = "Grundmodus";
      }
      if (this.steering_mode == "1") {
        res = "Warten auf Pumpenaktivierung";
      }
      if (this.steering_mode == "2") {
        res = "Pumpe aktiviert, Minimalpumpdauer aktiv";
      }
      if (this.steering_mode == "3") {
        res = "Pumpe aktiviert, warten auf Temperaturerreichung";
      }
      if (this.steering_mode == "4") {
        res = "Pumpe deaktiviert, Schutzzeit bis nächste Aktivierung";
      }
      return res;
    }
  },

  methods: {
    formatSeconds(time) {
      let sec = Math.floor(time / 1000);
      var minutes = Math.floor(sec / 60);
      var seconds = sec - minutes * 60;
      let res =  minutes + ':' + ((seconds < 10) ? '0' : '') + seconds;
      return res;
    },

    saveSetting(method, data) {
      console.log("method", method, data);
      fetch('//' + host + '/' + method + '?value=' + data)
        .then((res) => res.json())
        .then((response) => {
          this.updateData(response);
        });
    },

    updateData(response) {
      // console.log("response", response);

      this.temp_vorlauf = response.temp_vorlauf;
      this.temp_ruecklauf = response.temp_ruecklauf;
      this.temp_raum = response.temp_raum;
      this.switch1_state = response.switch1_state;
      this.working_mode = response.working_mode;

      this.MINIMALPUMPDAUER = response.MINIMALPUMPDAUER;
      this.MAXIMALPUMPDAUER = response.MAXIMALPUMPDAUER;
      this.PUMPENSCHUTZZEIT = response.PUMPENSCHUTZZEIT;
      this.TARGETDIFFERENCE = response.TARGETDIFFERENCE;
      this.RUECKTARGETTEMP = response.RUECKTARGETTEMP;
    },

    fetchData() {
      fetch('//' + host +'/get')
        .then((res) => res.json())
        .then((response) => {
          this.updateData(response);
        });
    },

    onWorkingModeClick(method) {
      console.log("method", method);
      fetch('//' + host + '/' + method)
        .then((res) => res.json())
        .then((response) => {
          this.updateData(response);
        });
    }
  },

  mounted() {
    this.fetchData();
  }
}
</script>
