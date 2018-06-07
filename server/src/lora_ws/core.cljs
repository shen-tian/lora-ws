(ns lora-ws.core
  (:require [cljs.nodejs :as nodejs]))

(nodejs/enable-util-print!)

(defonce serial-port (nodejs/require "serialport"))
(defonce express (nodejs/require "express"))
(defonce body-parser (nodejs/require "body-parser"))
(defonce http (nodejs/require "http"))

(defonce port (serial-port "/dev/cu.usbmodem1411" (clj->js {:baudRate 9600})))

(comment (.write port (->> {:lat -33 :lon 18}
                           clj->js
                           (.stringify js/JSON))))

;; app gets redefined on reload
(def app (express))

(.use app (.json body-parser))

;; routes get redefined on each reload
(.get app "/"
      (fn [req res]
        (.send res "Hello world")))

(.post app "/msg"
       (fn [req res]
         (let [body (js->clj (.-body req) :keywordize-keys true)
               json (->> body
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
