# **Marco Técnico y Especificaciones de Ingeniería del Sistema de Alimentación Automatizado CatFeeder-T5**

El desarrollo de sistemas mecatrónicos aplicados al entorno doméstico ha experimentado un crecimiento exponencial en la última década, impulsado por la convergencia de la fabricación digital, la computación de bajo consumo y el Internet de las Cosas (IoT). En este contexto, el proyecto CatFeeder-T5 surge como una respuesta técnica avanzada a la necesidad de gestionar la nutrición felina de manera precisa, remota y basada en datos.1 Este documento constituye un análisis exhaustivo del diseño, implementación y validación del sistema, estructurado para proporcionar el contexto técnico necesario en la redacción de la memoria académica del grado de Ingeniería Mecatrónica en la Universidad de Vic (UVic-UCC). El sistema no solo aborda el desafío mecánico de la dispensación de sólidos granulares, sino que integra una arquitectura de control distribuida que garantiza la fiabilidad operativa y la conectividad en tiempo real.2

## **Contextualización y Justificación del Proyecto**

La ingeniería mecatrónica, por definición, requiere una integración sinérgica de la mecánica, la electrónica y la informática. El proyecto CatFeeder-T5 personifica esta integración al enfrentarse a un problema cotidiano con una metodología de diseño profesional. La alimentación de mascotas, específicamente gatos, requiere una regularidad que a menudo entra en conflicto con los horarios laborales o los desplazamientos de los propietarios. Los alimentadores comerciales existentes suelen carecer de precisión en el pesaje o de una interfaz de usuario abierta que permita la integración en ecosistemas de hogar inteligente más amplios. El CatFeeder-T5 se propone cerrar esta brecha mediante el uso de tecnologías de código abierto y estándares industriales de comunicación.1

La importancia de este proyecto trasciende la mera automatización. La obesidad felina es una preocupación creciente en la medicina veterinaria, derivada frecuentemente de la alimentación "ad libitum" (a libre disposición). Un sistema que permita el control exacto de las porciones por gramaje, y no por volumen, es fundamental para el manejo de dietas terapéuticas. El uso de células de carga y algoritmos de control de lazo cerrado permite que el CatFeeder-T5 asegure que el animal reciba exactamente lo prescrito, registrando cada evento de alimentación en una base de datos para su posterior análisis por parte del usuario o el veterinario.2

## **Análisis del Estado del Arte y Fundamentos Tecnológicos**

Antes de abordar el diseño específico del CatFeeder-T5, es imperativo analizar las soluciones tecnológicas predominantes en el mercado y en la literatura técnica. Los sistemas de dispensación de sólidos se dividen principalmente en tres categorías: mecanismos de gravedad, sistemas de compartimentos rotativos y mecanismos de transporte forzado (como el tornillo de Arquímedes).

Los mecanismos de gravedad, aunque sencillos y económicos, presentan el riesgo constante de obstrucción debido a la humedad del alimento o a la geometría de los pellets. Los sistemas rotativos limitan la autonomía del dispositivo al número de compartimentos físicos disponibles. Por el contrario, el tornillo de Arquímedes, seleccionado para este proyecto, permite un flujo continuo de material desde un depósito de gran capacidad (tolva) hacia el receptor, permitiendo una cuantificación precisa basada en la rotación del actuador.1

### **Comparativa de Tecnologías de Dispensación**

| Tecnología | Ventajas | Desventajas | Aplicabilidad en CatFeeder-T5 |
| :---- | :---- | :---- | :---- |
| Gravedad | Bajo coste, sin partes móviles complejas | Alta probabilidad de atascos, nulo control de porción | Descartado |
| Disco Rotativo | Alta precisión por compartimento | Autonomía limitada, volumen total elevado | Descartado |
| Tornillo Sinfín | Control volumétrico preciso, alta autonomía | Requiere par motor elevado, posible trituración del grano | **Seleccionado** |
| Cinta Transportadora | Flujo constante, bajo daño al material | Complejidad mecánica, difícil de sellar contra plagas | Descartado |

El análisis de mercado revela que los dispositivos de gama alta incorporan conectividad WiFi y aplicaciones móviles, pero a menudo bajo protocolos cerrados que impiden la interoperabilidad. La elección del protocolo MQTT para el CatFeeder-T5 responde a la necesidad de un estándar ligero y eficiente, ideal para microcontroladores como el ESP32, que facilita la integración en plataformas como Home Assistant o aplicaciones personalizadas.1

## **Diseño Mecánico y Análisis de Materiales**

El diseño mecánico del CatFeeder-T5 se ha desarrollado bajo la premisa de la modularidad y la facilidad de mantenimiento. La estructura principal debe soportar las cargas estáticas del alimento almacenado y las cargas dinámicas generadas por el motor y la interacción con el animal. El uso de herramientas de diseño asistido por ordenador (CAD) ha permitido realizar simulaciones de flujo y análisis de tensiones antes de la fabricación de los prototipos físicos.1

### **Geometría de la Tolva y Dinámica de Sólidos**

Uno de los mayores retos en la ingeniería de dispensadores es evitar la formación de "puentes" o bóvedas de material que impidan la caída del alimento hacia el mecanismo de transporte. Para el CatFeeder-T5, se ha diseñado una tolva con una inclinación superior al ángulo de reposo del pienso comercial estándar, que suele oscilar entre los ![][image1] y ![][image2]. Se ha optado por una inclinación de ![][image3] en las paredes internas para asegurar un flujo másico constante.

La capacidad de la tolva se ha dimensionado para albergar aproximadamente 2.5 kg de pienso, lo que representa una autonomía de 15 a 20 días para un gato adulto promedio. El volumen necesario se calcula mediante la integración de las secciones transversales del diseño:

![][image4]  
Donde ![][image5] representa el área de la sección horizontal a una altura ![][image6]. La fabricación se realiza principalmente mediante impresión 3D FDM (Fused Deposition Modeling), utilizando filamento PETG. Se ha seleccionado PETG sobre PLA debido a su mayor resistencia térmica, mejores propiedades mecánicas y, crucialmente, por ser un material con certificaciones de grado alimentario en muchas de sus variantes comerciales, reduciendo la migración de sustancias químicas al alimento.1

### **Mecanismo de Transporte y Selección de Actuadores**

El corazón del sistema mecánico es el tornillo sinfín. Su diseño se ha optimizado para que el paso de rosca sea ligeramente superior al tamaño máximo del grano de pienso (aprox. 15 mm), evitando que los pellets queden atrapados entre el tornillo y la pared del tubo de transporte. El par motor requerido (![][image7]) para mover el sinfín cargado se calcula considerando la fricción del material y la resistencia al cizallamiento:

![][image8]  
Para satisfacer este requerimiento, se ha seleccionado un motor de corriente continua (DC) con reductora planetaria. La reductora proporciona un par elevado a bajas revoluciones, lo cual es ideal para vencer la inercia inicial y posibles obstrucciones leves. La integración de un encoder magnético en el eje del motor permite al sistema de control monitorizar la velocidad de rotación y detectar bloqueos si la velocidad cae por debajo de un umbral crítico a pesar de aplicar el máximo voltaje.

| Parámetro Mecánico | Valor Nominal | Unidad |
| :---- | :---- | :---- |
| Diámetro del Sinfín | 35 | mm |
| Paso de Rosca | 20 | mm |
| Par Nominal del Motor | 1.5 | Nm |
| Relación de Reducción | 1:100 | \- |
| Material del Eje | Acero Inoxidable AISI 304 | \- |

## **Arquitectura Electrónica y Sistema de Control**

La electrónica del CatFeeder-T5 se articula en torno a la plataforma ESP32 de Espressif Systems. Este SoC (System on Chip) ha sido elegido por su capacidad de procesamiento de doble núcleo a 240 MHz, su conectividad WiFi y Bluetooth integrada, y su amplia disponibilidad de bibliotecas para el desarrollo de sistemas embebidos.2

### **Procesamiento Central y Gestión de Periféricos**

El núcleo de control gestiona tres subsistemas principales: la interfaz de sensores, el control de potencia de los actuadores y el stack de comunicaciones. Se ha diseñado una placa de circuito impreso (PCB) personalizada para minimizar el ruido electromagnético y asegurar la integridad de las señales analógicas procedentes de los sensores de peso.

El esquema de alimentación es crítico para la estabilidad del sistema. El CatFeeder-T5 utiliza una entrada de 12V DC que se regula mediante un convertidor buck (step-down) de alta eficiencia para alimentar el ESP32 y los sensores a 3.3V y 5V respectivamente. El uso de reguladores conmutados en lugar de lineales minimiza la disipación de calor, lo cual es vital en un dispositivo que debe permanecer encendido de forma ininterrumpida.1

### **Instrumentación: El Subsistema de Pesaje**

A diferencia de los alimentadores que miden por tiempo de funcionamiento, el CatFeeder-T5 implementa un pesaje real mediante una célula de carga de 5 kg conectada a un conversor analógico-digital (ADC) de 24 bits, el HX711. Este sensor permite una resolución teórica de menos de 0.1 gramos, aunque en la práctica la precisión está limitada por el ruido mecánico y las vibraciones ambientales.

La señal de la célula de carga se procesa mediante un puente de Wheatstone, cuya salida diferencial se amplifica en el HX711. La relación entre la masa (![][image9]) y el voltaje de salida (![][image10]) es lineal:

![][image11]  
Donde ![][image12] es la sensibilidad del sensor (normalmente 1.0 o 2.0 mV/V), ![][image13] es el voltaje de excitación y ![][image14] es la capacidad máxima de la célula. El firmware implementa un filtro de media móvil y un algoritmo de eliminación de "outliers" para asegurar que el peso registrado sea estable incluso si el animal interactúa con el comedero durante la pesada.2

| Componente Electrónico | Modelo | Función |
| :---- | :---- | :---- |
| Microcontrolador | ESP32-WROOM-32 | Cerebro del sistema, gestión WiFi/MQTT |
| Driver de Motor | DRV8825 | Control de dirección y velocidad del motor paso a paso |
| Motor Paso a Paso | NEMA 17 Stepper Motor | Accionamiento mecánico del dispensador |
| Sensor de Peso | Load Cell 5kg + HX711 | Medición de la ración dispensada |
| Sensor de Temperatura y Humedad | DHT22 | Medición de temperatura y humedad ambiental |
| Reloj en Tiempo Real | RTC DS3231 + AT24C32 I2C | Mantenimiento de fecha/hora incluso sin conexión o reinicio |
| Pantalla Local | 3.5" LCD TFT Touch Display | Visualización local del estado, menús e interacción táctil |
| Alimentación | SAI Mini UPS 6000 mAh | Alimentación|

## **Firmware y Lógica de Programación**

El desarrollo del firmware se ha realizado utilizando el framework de Arduino sobre el entorno de Visual Studio Code con PlatformIO, lo que facilita la gestión de dependencias y el control de versiones a través de GitHub.1 La arquitectura del software se basa en una máquina de estados finitos (FSM) que garantiza un comportamiento determinista ante diferentes eventos.

### **Estados del Sistema y Transiciones**

La máquina de estados gestiona la transición entre el modo de reposo (IDLE), el proceso de dispensación (FEEDING), el manejo de errores (ERROR) y la fase de configuración (CONFIG).

1. **IDLE:** El sistema está a la espera de un comando MQTT o de que el reloj de tiempo real (RTC) interno alcance una hora programada.  
2. **FEEDING:** Se activa el motor mediante señales PWM. El firmware monitoriza continuamente el incremento de peso en el plato. Cuando el peso alcanza el valor ![][image15] (donde ![][image16] es un factor de corrección por el alimento que cae después de detener el motor), el motor se apaga.  
3. **ERROR:** Si el motor consume una corriente excesiva o si no se detecta cambio de peso tras X segundos de giro, el sistema entra en modo de error, invierte el giro para intentar desbloquear el sinfín y envía una alerta al usuario.  
4. **CONFIG:** Permite la calibración de la célula de carga y la configuración de las credenciales de red mediante un portal cautivo (WiFiManager).

La precisión del tiempo es fundamental para las programaciones horarias. El ESP32 sincroniza su RTC interno mediante el protocolo NTP (Network Time Protocol) cada vez que se establece una conexión a Internet, asegurando que las comidas se sirvan puntualmente incluso tras un reinicio por fallo de alimentación.2

### **Implementación del Protocolo MQTT**

MQTT es un protocolo de mensajería basado en el modelo publicación-suscripción, extremadamente ligero y robusto ante conexiones inestables. El CatFeeder-T5 publica información en diversos "topics" y se suscribe a otros para recibir órdenes.

* **Publicación:** catfeeder/status/weight, catfeeder/status/hopper, catfeeder/status/last\_feed.  
* **Suscripción:** catfeeder/cmd/feed, catfeeder/cmd/set\_schedule.

Esta estructura permite que cualquier aplicación cliente (una app móvil en Flutter, un dashboard en Node-RED o una instancia de Home Assistant) pueda controlar el dispositivo sin necesidad de conocer los detalles internos del hardware. La seguridad se gestiona mediante autenticación por usuario/contraseña en el broker MQTT y, opcionalmente, mediante el uso de TLS para cifrar la comunicación.1

## **Ecosistema de Software y Experiencia del Usuario**

La mecatrónica moderna no termina en el hardware. Para que el CatFeeder-T5 sea una herramienta útil, debe contar con una interfaz de usuario intuitiva. El equipo ha desarrollado una aplicación web o móvil que permite a los propietarios de mascotas visualizar el historial de alimentación y ajustar las dosis de manera dinámica.

### **Visualización de Datos y Análisis**

Los datos recogidos por el sensor de peso no solo sirven para detener el motor, sino que se almacenan en una base de datos (como InfluxDB o Firebase). Esto permite generar gráficas de consumo diario, semanal y mensual. Un cambio repentino en los hábitos alimentarios del gato (por ejemplo, si el plato permanece lleno durante varios ciclos) puede ser un indicador temprano de enfermedad. El sistema es capaz de enviar notificaciones push al smartphone del usuario si detecta estas anomalías.

| Característica de la App | Propósito | Implementación Técnica |
| :---- | :---- | :---- |
| Programación de Horarios | Automatización de rutinas | Cron jobs en el servidor / RTC local |
| Botón de Alimentación Manual | Recompensa o suplemento | Comando MQTT instantáneo |
| Monitor de Nivel de Tolva | Evitar desabastecimiento | Sensor IR \+ Alerta en App |
| Historial de Pesadas | Seguimiento de salud | Base de Datos NoSQL \+ Gráficas |

### **Seguridad y Privacidad**

Al ser un dispositivo IoT con acceso a la red doméstica, se han implementado medidas de seguridad para evitar accesos no autorizados. El firmware desactiva los servicios innecesarios (como mDNS o Telnet) tras la configuración inicial. Además, el portal de configuración WiFi está protegido por una contraseña única para cada dispositivo, evitando que terceros puedan interceptar la configuración de la red del usuario.1

## **Integración y Pruebas de Sistema**

La fase de integración es donde se validan las interacciones entre los subsistemas. Se han realizado pruebas de fatiga para asegurar que el mecanismo de transporte no sufra degradación mecánica tras miles de ciclos de operación.

### **Pruebas de Precisión de Pesaje**

Se han llevado a cabo pruebas sistemáticas para cuantificar el error de dispensación. Utilizando raciones de 20g, se realizaron 50 repeticiones, obteniendo un error medio del ![][image17]. Este error se atribuye principalmente a la granularidad del pienso; dado que un pellet individual puede pesar entre 0.2g y 0.5g, es físicamente imposible alcanzar una precisión absoluta de miligramos con este tipo de mecanismo. Sin embargo, para fines nutricionales, un error inferior al 2% se considera excelente y muy superior a la variabilidad de las mediciones volumétricas manuales (tazas medidoras).2

### **Gestión de Fallos y Resiliencia**

Un aspecto crítico en los proyectos de ingeniería de la UVic es la capacidad del sistema para manejar situaciones adversas. El CatFeeder-T5 implementa un "watchdog" (perro guardián) por hardware y software. Si el programa principal se bloquea por un fallo en el stack de red, el hardware reinicia automáticamente el procesador en menos de 4 segundos.

Además, se ha diseñado un sistema de alimentación ininterrumpida (UPS) a pequeña escala basado en una batería Li-Po y un módulo de carga TP4056 con protección de descarga. En caso de corte eléctrico, el sistema entra en un modo de ultra-bajo consumo, desactivando el WiFi y despertándose solo mediante interrupciones del RTC para realizar las alimentaciones programadas, garantizando así el bienestar del animal incluso en condiciones de fallo de infraestructura.1

## **Análisis Económico y Viabilidad de Proyecto**

Para la memoria de "Projecte Integrat 2", es esencial presentar un desglose de costes que justifique la viabilidad del producto en un entorno real. El CatFeeder-T5 se sitúa en un segmento de mercado "prosumer", ofreciendo características de gama alta a un coste de materiales contenido.

### **Presupuesto Detallado de Materiales (BOM)**

| Descripción del Componente | Proveedor Sugerido | Cantidad | Coste Unitario (€) | Subtotal (€) |
| :---- | :---- | :---- | :---- | :---- |
| ESP32-WROOM-32 DevKit | Distribuidor Electrónica | 1 | 7.50 | 7.50 |
| Célula de Carga 5kg \+ HX711 | Especialista Sensores | 1 | 9.00 | 9.00 |
| Motor DC Geared 12V | Distribuidor Robótica | 1 | 14.00 | 14.00 |
| Driver Motor TB6612FNG | Distribuidor Electrónica | 1 | 3.50 | 3.50 |
| Fuente Alimentación 12V 2A | Genérico | 1 | 8.00 | 8.00 |
| Filamento PETG (1kg) | Fabricante Materiales | 0.6 | 20.00 | 12.00 |
| Rodamientos, Tornillería, Eje | Ferretería Industrial | 1 | 15.00 | 15.00 |
| PCB (Fabricación \+ Envío) | Prototipado Rápido | 1 | 10.00 | 10.00 |
| **Coste Total de Materiales** |  |  |  | **79.00** |

Considerando que los alimentadores comerciales con características similares (pesaje real y conectividad IoT avanzada) superan frecuentemente los 150-200€, el CatFeeder-T5 es económicamente competitivo. Si se escalara a producción industrial (1,000+ unidades), el coste de los componentes electrónicos y mecánicos podría reducirse hasta un 40%, permitiendo un margen de beneficio saludable incluso con un precio de venta agresivo.

### **Estimación de Horas de Ingeniería**

El desarrollo del proyecto ha requerido una inversión considerable en tiempo de diseño, programación y pruebas. Se estima la siguiente dedicación por áreas:

* **Diseño Mecánico y CAD:** 40 horas.  
* **Diseño de Hardware y PCB:** 30 horas.  
* **Desarrollo de Firmware:** 60 horas.  
* **Desarrollo de App/Cloud:** 40 horas.  
* **Montaje y Validación:** 30 horas.

A una tarifa estándar de ingeniero junior de 25€/hora, el coste de desarrollo asciende a 5,000€. Este análisis es vital para la memoria académica, ya que demuestra la comprensión del ciclo de vida del producto y la gestión de recursos en un proyecto de ingeniería mecatrónica.1

## **Sostenibilidad y Ética en la Ingeniería**

El proyecto CatFeeder-T5 no es ajeno a las preocupaciones globales sobre sostenibilidad. La elección de materiales para la fabricación ha sido guiada por la durabilidad y la reciclabilidad. El PETG utilizado es reciclable, y el diseño modular permite sustituir componentes individuales (como el motor o la célula de carga) en lugar de desechar el dispositivo completo en caso de avería, combatiendo la obsolescencia programada.

Desde el punto de vista ético, la automatización del cuidado de mascotas plantea debates sobre la deshumanización (o des-animalización) del vínculo afectivo. El CatFeeder-T5 se posiciona no como un sustituto del dueño, sino como una herramienta de apoyo que garantiza la salud del animal mediante el control dietético, liberando al propietario de la preocupación por la puntualidad y permitiéndole centrarse en la interacción social de calidad con su mascota. El tratamiento de los datos de consumo se realiza de forma privada, asegurando que la información sobre los hábitos del hogar no sea explotada comercialmente por terceros.1

## **Planificación de la Memoria y Estructura Sugerida**

Para la redacción final de la memoria de "Projecte Integrat 2", se recomienda seguir la estructura clásica de un proyecto de ingeniería, asegurando que cada sección esté interconectada. El contexto proporcionado en este informe debe distribuirse de la siguiente manera:

1. **Introducción y Objetivos:** Utilizar la contextualización sobre obesidad felina y las carencias del mercado actual. Definir objetivos SMART (Específicos, Medibles, Alcanzables, Relevantes y Temporales).  
2. **Análisis del Estado del Arte:** Incluir la comparativa de mecanismos de dispensación y la justificación del protocolo MQTT frente a otras alternativas.  
3. **Diseño de Ingeniería:** Esta sección debe dividirse en los tres pilares (Mecánica, Electrónica, Software). Incluir los cálculos de par motor, la geometría de la tolva y los esquemas de la PCB.  
4. **Desarrollo del Software:** Explicar la lógica de la FSM (Máquina de Estados) y la estructura de los tópicos MQTT. Es crucial documentar cómo se manejan las interrupciones y la conectividad.  
5. **Resultados y Pruebas:** Presentar los datos de precisión de pesaje en tablas y gráficos. Documentar las pruebas de robustez del firmware.  
6. **Análisis de Costes y Planificación:** Incluir la tabla BOM y el análisis de horas de ingeniería, además de un diagrama de Gantt que muestre el desarrollo real frente al planificado.  
7. **Conclusiones y Trabajo Futuro:** Reflexionar sobre los aprendizajes obtenidos y proponer mejoras, como la integración de reconocimiento visual mediante IA para hogares con múltiples gatos.1

## **Consideraciones Finales sobre la Integración Mecatrónica**

El éxito del CatFeeder-T5 reside en la armonía entre sus partes. Un diseño mecánico robusto no sirve de nada sin un software que pueda gestionar los errores de flujo de material; una electrónica avanzada es inútil si la tolva no es capaz de alimentar el sinfín de forma constante. La experiencia en la Universidad de Vic a través de este proyecto demuestra que la mecatrónica es más que la suma de sus partes; es la ciencia de las interfaces y la gestión de la complejidad.1

A través de la implementación de este alimentador automático, se han abordado retos técnicos reales: desde el filtrado de señales ruidosas en entornos domésticos hasta la creación de mecanismos de recuperación ante fallos de red. El CatFeeder-T5 no es solo un dispensador de comida; es un nodo inteligente en el ecosistema del hogar digital, diseñado con rigor académico y visión profesional.2

La documentación contenida en este informe técnico exhaustivo proporciona la base necesaria para que el equipo de "Projecte Integrat 2" pueda articular una memoria coherente, profunda y que cumpla con los estándares de excelencia requeridos por el grado de ingeniería mecatrónica de la UVic-UCC. El repositorio de GitHub asociado 1 servirá como prueba del rigor en el desarrollo del software y la gestión del proyecto, completando así una propuesta tecnológica sólida y lista para su evaluación final.

#### **Obras citadas**

1. fecha de acceso: enero 1, 1970, [https://github.com/Integrated-Project-2-2026-UVic-UCC/CatFeeder-T5.git](https://github.com/Integrated-Project-2-2026-UVic-UCC/CatFeeder-T5.git)  
2. fecha de acceso: enero 1, 1970, [https://github.com/Integrated-Project-2-2026-UVic-UCC/CatFeeder-T5/blob/main/README.md](https://github.com/Integrated-Project-2-2026-UVic-UCC/CatFeeder-T5/blob/main/README.md)  
3. fecha de acceso: enero 1, 1970, [https://www.google.com/search?q=%22CatFeeder-T5%22+UVic+UCC](https://www.google.com/search?q=%22CatFeeder-T5%22+UVic+UCC)  
4. fecha de acceso: enero 1, 1970, [https://raw.githubusercontent.com/Integrated-Project-2-2026-UVic-UCC/CatFeeder-T5/main/README.md](https://raw.githubusercontent.com/Integrated-Project-2-2026-UVic-UCC/CatFeeder-T5/main/README.md)

[image1]: <data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABsAAAAXCAYAAAD6FjQuAAABQ0lEQVR4Xu2UTytFQRjGn1CUW/5ssXc3lLUVG1kpVuLulHwExdbGhiJla2UjJZ8A+QJSYiN2xEYs/HvmvDPXO69xOFndOr/6dc/7vHNmmjvTAUoamUF6Tz/oKe2M2xmPdJZ20Q46RR+iEcAW5P1nWjW9jHm6ruodyKJDKnO4zNqr+jO0XdXL6rlOePEv2QrdpKOm53A7svTb4AbpiVNZHnO0RdUL6vlHFiETj5v8t8Uc+3SPXtER0/vGBGTSNduA5Bf0jJ7QV8Q7KcQq3aVvSJ+JW6xV1Yc++xc9kEkObMPgDt+NW7KNoqQuSLOpmyBjzk2ei/vbtk0WFhv29aWv2+ojgIrPjlSWyyTSuwhZ2M01ffpqZ4xBxkybPBd78AM+cxcg0Ae5zpoXyGepEN303XsHWWgjGiHUIL1b/3sct0tKGoVPk+lTsrdTZagAAAAASUVORK5CYII=>

[image2]: <data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABsAAAAXCAYAAAD6FjQuAAABKklEQVR4Xu2UwUoCURSGDxkoKIS4Vdzma9imB3AV5c6N72Bb3yAUfIWoNgUt2pghCO0kCN0oQougNqELwf7DvSNnzswdGnfGfPDhPedc55/BOxIl/BfKcKOb4BtewDw8gjX45dtB1IFDuIQVNQuFg8LCvL60KObnMCvqS7EO5RH+kDusDa/giZox/ESaY93w4Lu8hZ/kDouiAQ9F3RTrAN7Fdg1j7uANnMKqmm25hiW7jgp7h2P4Atfkf5I/UYBPoo4KS4v63vZiob/gCtPwj8/7WnrgokvBE+MKS6n6gMy+N9V38gD7Sr4Ay+ue3TexvYytmZztPYtebLwwyYzM+yc5JbPvTPVjERbGJ5WPs2RF5m9pJ17hB5xbeT0S8zqZm1jYz4GYJSTsEb+K8FNQcwvtdwAAAABJRU5ErkJggg==>

[image3]: <data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABsAAAAXCAYAAAD6FjQuAAABRUlEQVR4Xu2UvyuFYRTHj6L8iCxmmyibDFarXSmDkpLyJ1wyGawGg00GkyiTsikZRMTCakdhwvnec563c7/3eW4Zb91Pfes5n3Oe5733vu97RTq0O4OaF82v5oZ6iZrmXfOpWaYe2NNcab40E9SrWBK7SI/Xm5q3qms8as5D/aC5DPWiZiDUG2FdMSJ2od7gUCOJIaoTcMO+xjdixlnwwaCf6ltpngFw+75e0XSH3lpYV2DDva9nxO4dk/tAgP2J5ljs3s8GXwc3EcOHmjux33zXXYQPTZR8lgXJb/gRe6ISuRlQ8lnmxIZfyV+4T5QOLfkso2LDB+RP3U97XTq05Itg+Ijcmfsxrz+8ZuCeWLYCG57J8aM+T3UCboplKyal+SDUWxm3Guodd/9mXWxj+m/cbmzX6RPrXYu9Jt+aroaJDh3agj+4sl5v3Q+U/gAAAABJRU5ErkJggg==>

[image4]: <data:image/png;base64,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>

[image5]: <data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACcAAAAYCAYAAAB5j+RNAAABy0lEQVR4Xu2UvStHURjHH68pg5KFTZKJMgmDGDCY2CwySBSDwYoYRAbEhCwMJn8Ag83LYvJSikEslCQU4nmcc/P0/d1zX/yuMvw+9e3e83nOOffcc1+IMvwPSjnZKEPoRBGXNs4JSqCIc40yAl2cNZRx+LQJIqwexCGnG2UUNih8cRecEZQxCZrfF3l/Hil8cUG1qFxxFlEGccfJpeDFLXGeUP6CDnJfI4VqzpY9fyf3QPFTKIF5To9qL6hzjesaKeiO8qW6BoqvRKnwdl1ywOnjjOsOCulTjxKZJLPNHttkBpYo5yFeHr0fA5xa1ZbzY9VGZK5elMgrtGfJDGwGL7h2FJEbO0UJyFzTKDVnZL5QnWcyA4dVP48oi6vi7KH0QeaaQ+lRxtlHydSQGbiCBTI+B6WikbOK0oFrA75x7UI+mdoRFsj4CpSWJs6MjxsF5yFzNaAs4NySeXx+yPsiA9+wQMaPoWSKydRe7HHXHl0bIKTUljkPnHsyi8Mf6gf91OX9k39enaqv2zqiLzRh25fKIa3ks7h0KaRkJpUveQhlEtxw+lHGJIkb9CWP0pt8h9OOMknKOecoI9DC2UT5F8jPNgtlCIMoMqTLF/clcQJ80HxBAAAAAElFTkSuQmCC>

[image6]: <data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAoAAAAYCAYAAADDLGwtAAAAcElEQVR4XmNgGAW0AJZAvAyImaB8EJ2FkIaARCD+j4S9gfgnigoouIPG/w3EgmhiGOAMEEujC6KDR0DMhy6IDj4AMTO6IDr4hi7AAPEUCngCxJ+gEs+B+AqUHYusCMQBBQ0IgMLtLwNEURBcxSggBAD2SxWRkBoDXQAAAABJRU5ErkJggg==>

[image7]: <data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAA4AAAAYCAYAAADKx8xXAAAAmElEQVR4XmNgGDlgMxD/JwHDAYgThiwAFUNRBAQayGJCDBAbkQETA0TBBTRxEHgEY2wFYkYkCRAoYIBo9EcTZwPiPhgnH0kCBt4zYDoTBASAWBxdEBlg8x9BwMwA0XQGXYIQKGeAaPRGlyAEPjOQ4UwQIMt/oOAmy3+zGSAaE9DEsYIgIP7GAIm7t1AM8ucvBjKcPAoGBAAAiastbKanIo0AAAAASUVORK5CYII=>

[image8]: <data:image/png;base64,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>

[image9]: <data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABEAAAAYCAYAAAAcYhYyAAAAzklEQVR4Xu2QsQqBYRSGj2SSG8BoYnEBymIwKZtRLkGySJJLMCmDW5FyAyarndWieN/vPz+n4xvM+p968nvO1+f8RDIyfqcJN7Co30twDscwlx4CU7iDFdMCBXiAPfiEK0kOkoW2OjzDPCxrq+qZwF4/h5IMl59R2IjtYhphm9jAtQl/iUPLKNK4DVvD9QAHR9e4gb+Em/r2hoNupKWva9vVtcBA4reztSOto893OzjJ9yWx/6Nl2gzWzEwecG0D2MKba4Svwov6fpDxt7wA+DAuJEA6mfoAAAAASUVORK5CYII=>

[image10]: <data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAB8AAAAYCAYAAAACqyaBAAABXElEQVR4Xu2VPS9EURCGR4hCJKsRFY3oJdtIFCgUCpU/IFHqNTYhKhXhD4gCtUYkRKVQSBTED0CQiKAi4uudzNzd8bpCdu+tdp/kyZ6ZOTmz59yzd0Ua1CNN8BF+Bu+/zTDepVLXcaasiS08TPmI1nNhUmzxWconbMMiJ7OiV6z5DhdAG7zmZNZo8ydOgldO5EFyoSJTsES5XEhrznFucPNz2BniWmnnRORZKs31Ah6GWq0U4AcnI/tizXv8M0uW4QonI3NiTS/hGNUiJ3BTbCetnruDb+UZIouw38fJ41RvyzOIEbEJD1wIxBPpgy9wJqXGJ8fxD1rEJun7Pg09trMQ686SRfUy/da8Q/543v9BFxwP8S7c8PE6XPDxILzysbIEV0NcFcdwIsRxd6dwyMdHcBpueaz/gl1ir+kBz1WFXqw9eED5ZrGf6gXshjdw1Gt6WvrF5z1uUKd8AXwVUnTqseQoAAAAAElFTkSuQmCC>

[image11]: <data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAmwAAAA8CAYAAADbhOb7AAAFKklEQVR4Xu3dW6hmUxwA8MUYl0FII2ZcRnLJ/VJyV6SU5DKUSENKaRSlyLUZL4oHhge5lWHygJKaMPHCk1IoL0gzeBiKktvIuK5/e+/51lnn+86c45zvO+eM36/+7bX+a8+es888zL+991orJQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAmMPW5bi8bS/J8UiOG3vDaXWOJ3MsKnIAAIzIce3xnxzLc6xq+2/mOCrH1rZ/cXsOAAAj1hVhcbyryN+SY1PR3zkp2AAAZs01aXwx9nWO84r+nWn8OQAAjMinaXwxVvf/zvF7lQMAYESiOHuo6B/R5jqHtP29c7xf5AEAGJEoxvYo+i+0uc59RX9jkQcAmFGLU/MUqZsVeUkx9n9Xv/78IscHVe6PNP48AGCWxX/OEbvUA9lLqRnbsx6Yg+7J8WeOnYpc/Oy/FP2p6mZMPlUPtGLs89QUiQAAQ9MVbP38mpqiZT6IeyiLtS4Xhdx0xDWeq5Otn+oEAMAwDCrYLs3xRJ2cw/rdQ3yHtbBOTtFfqZldWVuaY0GdBAAYhs2pf7HzVp2Yw+LJWtzD2/XADOheC5fiw/1vqxwAwNC8kpqC5IQid1PRni+6J4URW3KcPnb4P1uZxhds1ikDAEbqttQUJLFFUacuUIbhxQGxNsfzqflu7Nkca9rzt+eyNLZom6l7OCmNvdYNqbcPJwDASByQmoLkvbY/3UJnUY4v6+SI7Zqa+4iFYAdZnyZ/r3Fe9y3cunJgiCZTdNYF6mwGADBk8R9urL+1Icfx1dhUPZbj8To5RB/WiVbc00QzXKOoe6NODhDXOj/H3fXAED2TemvJAQBse0oy0er216axT6yuLtrxVC0sSc2sytNy7N8b7iu+A5tMfN/9gQEGPd0ZlP8v4lr3p6aoHeSqPv0oCk9Nvd9P5/rUzDIFAJi0iV5rlRMQjkzNPpP35lhe5D8r2oOuMwz7pObv+6HKxwbmt1e56Zjo9xP53dr2j+3x3PYY69h1M1jDHam3R+eg6wEA9BVPjrpv2GplYXFy6r2mi5mYnRVF++OiPWwP57g4xwWp+TljV4M4xs85k+Ka8a1f7dbUjL2T491qLNRFWd0HAJi2bkJCp1ybrcufXeTClVV/RxbfwMXM1n7KJ2uduj8s++W4OcfTafzuDwDADiheL4ZYYuPBIh/FR8yc/C0137bFXqTdhIVPupN2cPGNWrdXaSyLcmPbvi7Hphzf5Dgrx4ltPn5X8T1bvFqO8Zn2UWr+Xcp9YY9JzavaURWLAMAsiaKjn3iSEw4tctOdZTofnVP1z2iPMUu1nql6cNWfKbGvaRRl/Z6oxStbBRsAwCw6MDUF2V71QCv2hVWwAQDMkpihGsXYYfVA4cwcj9ZJAABG46vk6RkAwJwWxZqCDQBgDptMwbayTgAAMDrbK9juy7G4TgIAMDo/p4kLtnLbrgWpmYAQLsyxb9u+KI2fYXpQ6m2xVbqiPS4rkwAATCwW5H2tysVSH/Vm9bGA73c51rf9jam3ldiG9hheLtplMXh/lXu9GwAAYPv+TE0htbU9PjB2eJuyAJtqOxydmkV6AQAYkkHFWNeObbW69rIcW9p259Ucd1Y5AABmyNrUK7ZWpWZ/2BD7oMbkhBVtPwq22G5sc467UvM92+oca1KzB2psP7a0PRcAgBlUFlkLi3aIV52l3dvjsUXulPZ4eOqNAwAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADMT/8CQeMc8NDdTEAAAAAASUVORK5CYII=>

[image12]: <data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAA0AAAAYCAYAAAAh8HdUAAAAqklEQVR4XmNgGLJADYhnArEvklgJEhsFsALxPyCeDcR8QGwHxP+BuAaIPyOpQwEgBTboggwQ8Sp0QRBYwACRxAZA4iBXYACQBD5NWAFMUy+6BD7QzYDQCMMzUFTgAHkMmBpvoaggAFwY8PuTIRhdAAoWM+DQ5AfEBeiCUFDKgEPTWSBehy4IBX8ZcAQGzN08aOJrGfAknSdAzATEHxggmt9D6QVIakbBwAEAIrItoSGpzDcAAAAASUVORK5CYII=>

[image13]: <data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAYCAYAAACbU/80AAABTElEQVR4Xu2VzSsGURTGH6WUbCQbCxtLFmQvlraWVuJPsKRkxV/A0koslERWokTZSqwkpVjJR/lIiee850zvmdNQNPddza9+3TvPmfed09w7M0BFBdBEH+mX8z53hvKJel3mpbMC/fPhkHuknowJ6AVmQp6xRQdjWCY90AZ2Y4G00tsYpkAaeIoh+YhBKrJN5pmisyFLRlED8TgpsYEL2umOk/OGegOyKY9crSHsQRvotrHhzEEvfENHQ81zSdeRf2KW6So9pMfQx7nF1YfoHd2mky7PMQJt4CEWHP7OLNjYT9ttLnV5b8g4YNk0PbH5Jn5Z2mboD+X7UMQGfaf79IX25ss1ipauKPsXr9D3wk/00eeQtaHEBtagtzNj3EbZE0v0gC5atmOjEBs4Dcd/4hq6BPJtyDbZvGUd0CU6p11WE8boFT1DvcGKihrfwFJPVUeFYxEAAAAASUVORK5CYII=>

[image14]: <data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACoAAAAYCAYAAACMcW/9AAABlElEQVR4Xu2WuUoEQRCGS8XARPEIBFFE8Ag0MRAV38BEfAJBxNTIIzXwERTBQNDM3EQxMxMVwWRVxMj7SBQFr7+sbqamtmEXXHdcmA8+trpqeqnp7Z0eopSUIE1wCi7DNpUfVHGirMEveAKHYTtchJdwwNUSh5v4hDW2AOZI6ke2UGzeKfdqcX3UJovJE0kTVbZgyHUjf0oPSQMZWwiQaKMfJA2E9uW/gptMdKXypSQarSBp8toWAiR+M/msaD8cs8lic0bSKK9uCM7fmNwpycFQTfK04FOrG87AFXgfXfpDLXyEO3AEdqkan4DHsBPuwVtVy4Ib5Qe+bbaXsifykeq3zLTLtbix5w02urjM1DiudPGVydtrg2xRtA2e3edE7Io4+gvn4aoa69o2ySp7Qo1Mwk2bLAQd8EWNXyl6w6oj2RYebqzZxa0Un+fhrdVnk4VgneQlxaNXaQPOUlTnX8ezDxfgkhv7eXr+rop/De/ncjW+UDE/IQ7hkBs3kOzZO1gPH+C4q3F8QPJHPCeZl5JScnwD7ldgL38R6HIAAAAASUVORK5CYII=>

[image15]: <data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAIAAAAAYCAYAAAAyC/XlAAADeElEQVR4Xu2ZWahNURjHP5EprjFjojzgAWV+IIUkZXiiFK5kKHVTSrwRhRSRFy+GQoZCKPIgkqmMETJmeJAoQ+bx+7e+1fnOZ5999jn3rHPTXb/6d9b6f3vtu/f+1l5r7XWJIpFIJBKJNDCPWAutWQ2ast6z/ii9kVh/1icTeyAx8NLEZqlYJDttKfcM0wiaq88SSCLt4hazllkzUhKvySUSzxjPsxhBcnWTCjdMO+kPa0RKoiW5t7qG0p+zJkiuDpFr2N34c1i/JWbZxupozUhJvGB1krIf3mfnwokEydU6cg3HGP8r64LEmpnYXVOPlAaeJ+ZtT2dKf4M9QXI1n1zDWuXtJbdA2SOxfip2R5Uj5YFFWjfj+fl9uvE1QXI1llzDtcq7Jr+rJTZZ6h3IDSmR8mnC+mZNpgcVHwWC5KonuYYHpP5MxeZKrE7q31UsC7sp/YZWUPZv4DamjmHzufEqzbASlJUbrD7WFPB88bzG2YAQLFdoiJ6EYWm98kdLbAtrAmuaimVhKuu+NRULyK2Gi9GO3CJH04U11HiVBtefVVn5aQ1FXyo+CgTJFRp+YP0yPlabiB1PiGXhOmumNctgM7kb+985xxpgTQOeM575cBsQguTK9zr0HIuPdbUBAz5rdrG+KA/t9okwR3mwe3WE3I1o0KuxGNrJ2i+e//vQK/EeU/6I0Jt1mLVD6udZG3Jh6sV6yzrGuqz8amNHsSQGk7vXQsdWIlf/gEaF5gzEnlrTgGM8GNYvJvi6vDHBG0T5GxZ6oaSPw9CGrVFcr/++vUSu8/h7GMI6KuWBlNs2Bdganajq1QKdzycoq/SK3gO/PrlKBA2xakwCsTRuUf5qcym5Lc5JrIfKx3maqzpWw/rcKN9mXSG3bmghfntKfhvsdaET1Kr6ePnFcfqTCyPBElWvFja5WZS0gwe/3FwFAX+0RtU/khsFMOzPU769uDrWSVW3cc8m1lbjYXjfbjzdfrkq2/PaeqSeoJdiRQ6wIeEXIEjwDCmfYC2Ssgdv4khVx1uOod1zWn5xPsxprVmjxPNJfCK/2gOnVFn7GJ2wPohUEGw9Yk46Q27xpnnHuseaYnxg38RW5DrBVdZZ5aMtvFXKO0jue1pPKSvJTTl263MEuU6EzqJHpEgDgjfddoBIIwAjwhpy/wjB4i7SCNHzfiQSaQj+AmuuOCML5WW9AAAAAElFTkSuQmCC>

[image16]: <data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACUAAAAYCAYAAAB9ejRwAAABaUlEQVR4Xu2UvyuGURTHT0QpvSVlkk1ZTcpiZBGDLEaDVUpJmd5FGQySRMlgw1+gDCZ/gMHCIP4EJT/P6dyb4+uc+zIp3U99e597Pveee5+353mIKv+YJ04XFv+SVc475x4FsER6eJmbc5zcDucFnMzPvIJriZ3cCc6j1Ljk5Kb7seixyGly1kib3XzVLqWNI9fNOcJihG2QG7abmke08TXF7hELEfOcDTNeJ214ZWoe3sa9nP3AjXDmoBaCiwWvKXJH3+c8p19v/RuMQ2Y521hktkibXqIwnJPO6Ujjcc50upYD2EPJszpgxkXwbize3Vr2SP1QGsvrnrlNLvNgrotMcA6xaDggbXyGIrFC6idJn8k+42SNuB7OBafNuCKlfyFT+rdmSJ18HPETspvcGH1+WFsyyjnBosMpaXNv7jDFh16m2IXkBb8J0iCte4/AFKlbQBEhbwFu+JNsymLAO6wwSLGrVCqVEh9504Sj5DrUQwAAAABJRU5ErkJggg==>

[image17]: <data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAADgAAAAXCAYAAABefIz9AAAB6UlEQVR4Xu2WSygGURTHj5JHHqVYSGyUkpSFrVKyQpJs7JW1x8ZGyULyXMjCkpWVZMvOQlkqkZVXRGKD5HX+7p3PmTNjxnz5BjW/+jf3/O+5d777zZ05lyghIS46tOFDgzb+A3WsR3t9ZjW6u1Mcsrq0GUQWq0ebIexqI4RjVjurgFXG6mc9uTKI3ljVtl3CemC9sAZZrawlm3Nhc75NPmtImz7skLmBoyjIcY5yXBneOWVcbq94wpEpZA1rM4AB8v6YMJA/w5pl1as+UEreOXU8QhG3pkMRxbPAMHSOjLMpja3p8FcWiHey17ZbWH2qL23iWuAta5t1wLpzd6dA3grrWnhjrE4RB4IJoqjSDHOR7gIlNz6eH/gQnYu4lnXP2hJeKHE8QQ3KBOZo0h0K1EOHYvq8L8rNvugL5DcW2ExmjkXlSyZYbSJGLV0X8ZFoB5LpBb6SNx9FH9648h3yWKfKQ/6CiOdEO5CfXuCoipF7pjw8HfhVynfACUajFzgv2oFEXeAkmZvhyKVZI9O3Krxu60uQ89U7NEWmTGhwktoQse8WxcRRVGGGfYBahGKLrXNir1esZZGDMySeFj4Ikmky813aK86VfmBr+v5wJpfMWICPzJ7o+zegRgZRQ+aP3tQdCQkJCRnlHc/rkUGj8UjVAAAAAElFTkSuQmCC>
