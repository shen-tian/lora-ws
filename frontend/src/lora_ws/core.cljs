(ns lora-ws.core
  (:require [ajax.core :refer [POST]]
            [cljs.pprint :as pprint]
            [clojure.string :as str]
            [goog.crypt.base64 :as b64]
            [octet.core :as buf]
            [ol.layer.Tile]
            [ol.source.OSM]
            [ol.proj]
            [ol.Map]
            [reagent.core :as reagent]))

(def packet-spec
  (buf/spec :magic-0 buf/byte
            :magic-1 buf/byte
            :callsign (buf/string 4)
            :latitude buf/int32
            :longitude buf/int32
            :accurate? buf/byte))

(defn sent-packet
  [{:keys [callsign latitude longitude accurate?]}]
  (let [buffer  (buf/allocate (buf/size packet-spec))
        _       (binding [octet.buffer/*byte-order* :little-endian]
                  (buf/write! buffer {:magic-0   0x2c
                                      :magic-1   0x0b
                                      :callsign  callsign
                                      :latitude  (int (* latitude 1E6))
                                      :longitude (int (* longitude 1E6))
                                      :accurate? 0x00}
                              packet-spec))
        payload (-> (b64/encodeByteArray (js/Uint8Array. (.-buffer buffer)))
                    (str/replace "+" "-")
                    (str/replace "/" "_")
                    (str/replace "=" "."))]
    (prn latitude longitude)
    (POST "http://192.168.1.227/message"
          {:format     :text
           :url-params {:payload payload}})  ))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Vars

(defonce *state
  (reagent/atom {}))



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Page

(defn send!
  []
  (sent-packet {:longitude (js/parseFloat (:longitude @*state))
                :latitude  (js/parseFloat (:latitude @*state))
                :callsign  (:callsign @*state)}))

(defn packet-form
  []
  (fn []
    [:div.form
     [:div.form-group
      [:label.control-label "Callsign"]
      [:input.form-control {:value     (:callsign @*state)
                            :on-change #(swap! *state assoc :callsign (.-value (.-target %)))}]]
     [:div.form-group
      [:label.control-label "Latitude"]
      [:input.form-control {:value     (:latitude @*state)
                            :on-change #(swap! *state assoc :latitude (.-value (.-target %)))}]]
     [:div.form-group
      [:label.control-label "Longitude"]
      [:input.form-control {:value     (:longitude @*state)
                            :on-change #(swap! *state assoc :longitude (.-value (.-target %)))}]]
     [:pre (with-out-str (pprint/pprint @*state))]
     [:button.btn.btn-primary {:on-click send!}
      "Send"]]))

(defn page [ratom]
  [:div.container
   [:h1 "LoRa Gateway"]
   [:div.row
    [:div.col-sm
     [packet-form]]
    [:div.col-sm
     [:div#map {:style {:width "100%"}}]
     [:button.btn.btn-primary {:on-click #(js/initMap)}
      "Generate"]]]])

(defn ^:export set-lon-lat
  [lon lat]
  (swap! *state assoc :longitude lon)
  (swap! *state assoc :latitude lat)
  (send!))



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Initialize App

(defn dev-setup []
  (when ^boolean js/goog.DEBUG
    (enable-console-print!)
    (println "dev mode")
    ))

(defn reload []
  (reagent/render [page *state]
                  (.getElementById js/document "app"))
  (js/initMap))

(defn ^:export main []
  (dev-setup)
  (reload))
