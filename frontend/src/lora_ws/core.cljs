(ns lora-ws.core
  (:require [reagent.core :as reagent]
            [goog.crypt.base64 :as b64]
            [octet.core :as buf]
            [ajax.core :refer [GET POST]]
            [clojure.string :as str]))

(def packet-spec
  (buf/spec :magic-0 buf/byte
            :magic-1 buf/byte
            :callsign (buf/string 4)
            :latitude buf/int32
            :longitude buf/int32
            :accurate? buf/byte))

      ;; newMessage.replace("-", "+");
      ;; newMessage.replace("_", "/");
      ;; newMessage.replace(".", "=");

(defn sent-packet
  [{:keys [callsign latitude longitude accurate?]}]
  (let [buffer  (buf/allocate (buf/size packet-spec))
        _       (binding [octet.buffer/*byte-order* :little-endian]
                  (buf/write! buffer {:magic-0   0x2c
                                      :magic-1   0x0b
                                      :callsign  "ABCD"
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

(defonce app-state
  (reagent/atom {}))



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Page

(defn page [ratom]
  [:div
   "Welcome to reagent-figwheel."
   [:button {:on-click #(sent-packet {:longitude 18.416937
                                      :latitude  -33.923270
                                      })} "Send"]])



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Initialize App

(defn dev-setup []
  (when ^boolean js/goog.DEBUG
    (enable-console-print!)
    (println "dev mode")
    ))

(defn reload []
  (reagent/render [page app-state]
                  (.getElementById js/document "app")))

(defn ^:export main []
  (dev-setup)
  (reload))
