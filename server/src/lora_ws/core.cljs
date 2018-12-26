(ns lora-ws.core
  (:require-macros [hiccups.core :as hiccups :refer [html]])
  (:require [cljs.nodejs :as nodejs]
            [hiccups.runtime :as hiccupsrt]))

(nodejs/enable-util-print!)

(defonce serial-port (nodejs/require "serialport"))
(defonce express (nodejs/require "express"))
(defonce body-parser (nodejs/require "body-parser"))
(defonce http (nodejs/require "http"))

(defonce port (serial-port "/dev/cu.usbmodem1411" (clj->js {:baudRate 9600})))

;; app gets redefined on reload
(def app (express))

(.use app (.json body-parser))

(def page
  [:html
   [:head
    (include-css "mystyle.css")]
   [:h1 "LoRA Web"]
   [:form
    [:label "Hii"]
    [:input {:type :text}]]])

;; routes get redefined on each reload
(.get app "/"
      (fn [req res]
        (.send res (html page))))

(.post app "/msg"
       (fn [req res]
         (let [body (js->clj (.-body req) :keywordize-keys true)
               msg  {:callsign (or (:callsign body) "LORA")
                     :lat      (or (:lat body) 0)
                     :lon      (or (:lon body) 0)}
               json (->> msg
                         clj->js
                         (.stringify js/JSON))]
           (.write port json)
           (.send res "Worked!"))))

(defn -main []
  (.log js/console "Hiii")
  ;; This is the secret sauce. you want to capture a reference to
  ;; the app function (don't use it directly) this allows it to be redefined on each reload
  ;; this allows you to change routes and have them hot loaded as you
  ;; code.
  (doto (.createServer http #(app %1 %2))
    (.listen 3000)))

(set! *main-cli-fn* -main)
