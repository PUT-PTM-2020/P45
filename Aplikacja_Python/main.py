import math

from matplotlib.backends.backend_tkagg import (FigureCanvasTkAgg, NavigationToolbar2Tk)
from mpl_toolkits.axes_grid1 import host_subplot
from matplotlib.figure import Figure
import mpl_toolkits.axisartist as AA
import numpy as np
from tkinter import *
from tkinter.filedialog import askopenfilename


def is_number(string):
    if string == ".":
        return False
    for letter in string:
        if not letter.isdigit() and letter != ".":
            return False
    return True


root = Tk()
root.title("Hamownia drogowa")
root.resizable(False, False)
root.iconbitmap("./tire.ico")

width = 20
height = 1

fr1_a = Frame(root)
fr1_a.grid(column=0, row=0)

lbl_a = Label(fr1_a, text="Przed wybraniem pliku z danymi proszę zatwierdzić dane", font="Verdana 8 bold")
lbl_a.grid(column=0, row=0)

fr1_a1 = Frame(fr1_a)
fr1_a1.grid(column=0, row=1, pady=20)

filename = ""


def get_file():
    global filename
    filename = askopenfilename(title="Wybierz plik z danymi", filetypes=([("Pliki tekstowe", "*.txt")]))
    if filename != "":
        text_f1.set(filename)
    else:
        return False
    load_data()


class ChartDrawer:
    def __init__(self, datax, datay, color, style, labely):
        self.color1 = color
        
        self.figure = Figure(figsize=(8, 8), dpi=60)
        self.axes = self.figure.add_subplot(111)
        self.axes.plot(datax, datay, color + style, label="RPM")
        self.axes.set_ylabel(labely, labelpad=15, size=14, color=color)
        self.axes.tick_params(axis='y', labelcolor=color)
        self.axes.set_xlabel('czas [ms]', labelpad=15, size=14)
        self.axes.set_title('Wykres hamowania', pad=25, size=20)

    def add_values_new_ax(self, datax, datay, color, style, label):
        self.color2 = color
        
        self.axes2 = self.axes.twinx()
        self.axes2.plot(datax, datay, color + style)
        self.axes2.set_ylabel(label, labelpad=15, size=14, color=color)
        self.axes2.tick_params(axis='y', labelcolor=color)
    def add_values_axis(self, datax, datay, color, label):
        self.axes.plot(datax, datay, color)

    def add_values_axis2(self, datax, datay, color, label):
        self.axes2.plot(datax, datay, color)

    def load(self):
        try:
            self.canvas.get_tk_widget().destroy()
        except:
            pass

        self.canvas = FigureCanvasTkAgg(self.figure, master=fr1_b)
        self.canvas.get_tk_widget().pack()

Tg_arr = []
KM_arr = []

def load_data():
    if filename == "" or zatwierdzono is False:
        return False
    danex = []
    daney = []

    i = 0
    maxrpm = 0
    rpm_p = 0
    rpm_n = 0
    rpm_sr = 0

    file = open(filename, "r")
    for line in file:
        i += 1
        if i <= 2:
            continue
        temp = line.split(";")
        danex.append(int(temp[0]))
        daney.append(int(temp[1]))
        if int(temp[1]) > maxrpm:
            maxrpm = int(temp[1])

    przelozenie = kon_rpm / maxrpm
    i = i - 3

    for k in range(50):
        rpm = 0
        rpm_p = 0
        rpm_sr = 0
        for j in range(i):
            rpm = 0
            rpm_p = rpm_sr
            rpm_sr = daney[j]
            if rpm_sr - rpm_p < 0 and j <= i:
                rpm = (daney[j + 1] + daney[j - 1]) / 2
            else:
                rpm = rpm_sr
            daney[j] = math.ceil(rpm)
    if daney[i-1] > daney[i]:
        daney[i] = daney[i-1]

    danex1 = danex.copy()
    danex1.pop()

    global obkola, hopony, dkola, Tg_arr, masa, KM_arr
    Tg_arr = []
    KM_arr = []
    a = 0
    KM = 0
    for z in range(i):
        Tqkola = 0
        Tq = 0
        if z < i:
            a = math.fabs((daney[z + 1] - daney[z]) / 60 * obkola / 1000 / 0.2)
            Tqkola = masa * a * (dkola / 1000) / 2
            Tq = Tqkola / przelozenie
            KM = Tq * daney[z] * przelozenie / 9548 * 1.3636
            Tg_arr.append(Tq)
            KM_arr.append(KM)

    global fr1_b
    fr1_b = Frame(root)
    fr1_b.grid(column=1, row=0)

    global chart

    poly = np.polyfit(danex1, KM_arr, 10)
    KM_arr1 = np.poly1d(poly)(danex1)
    poly = np.polyfit(danex1, Tg_arr, 10)
    Tg_arr1 = np.poly1d(poly)(danex1)
    poly = np.polyfit(danex, daney, 10)
    daney1 = np.poly1d(poly)(danex)

    chart = ChartDrawer(danex1, KM_arr1, 'g', '', 'Konie mechaniczne')
    chart.add_values_new_ax(danex1, Tg_arr1, 'b', '--', 'Moment obrotowy')
    chart.add_values_axis(danex, daney1, 'r', 'RPM czujnika')
    chart.load()

    global text5, text6
    max_km = 0
    for km in KM_arr1:
        if km > max_km:
            max_km = km

    max_tq = 0
    for tq in Tg_arr1:
        if tq > max_tq:
            max_tq = tq

    text5.set(round(max_km, 2))
    text6.set(round(max_tq, 2))

btn0 = Button(fr1_a1, text="Przeglądaj", command=get_file, state="disabled")
btn0.grid(column=0, row=0)

btn1 = Button(fr1_a1, text="Przeładuj", command=load_data, state="disabled")
btn1.grid(column=1, row=0, padx=10)

text_f1 = StringVar()
text_f1.set("Nie wybrano pliku")
lbl_f1 = Label(fr1_a, textvariable=text_f1)
lbl_f1.grid(column=0, row=2)

fr1_a2 = Frame(fr1_a)
fr1_a2.grid(column=0, row=3, pady=30)

fr1_a2_b1 = Frame(fr1_a2)
fr1_a2_b1.grid(column=0, row=0)

lbl = Label(fr1_a2_b1, text="Masa (kg)")
lbl.pack(expand=True)
ent1 = Entry(fr1_a2_b1, width=width)
ent1.pack(padx=20)

lb2 = Label(fr1_a2_b1, text="Szerokość opony (mm)")
lb2.pack(expand=True)
ent2 = Entry(fr1_a2_b1, width=width)
ent2.pack()

lb3 = Label(fr1_a2_b1, text="Profil opony (%)")
lb3.pack(expand=True)
ent3 = Entry(fr1_a2_b1, width=width)
ent3.pack()

lb4 = Label(fr1_a2_b1, text="Średnica felgi (cal)")
lb4.pack(expand=True)
ent4 = Entry(fr1_a2_b1, width=width)
ent4.pack()

text_e = StringVar()
err = Label(fr1_a, textvariable=text_e)
err.configure(foreground="red")
err.grid(pady=10, row=4, column=0)

hopony = 0
dkola = 0
obkola = 0
masa = 0
pocz_rpm = 0
kon_rpm = 0

zatwierdzono = False

def buttonClick():
    global zatwierdzono

    zatwierdzono = False
    btn0["state"] = "disabled"
    btn1["state"] = "disabled"

    val1_s = ent1.get()
    val2_s = ent2.get()
    val3_s = ent3.get()
    val4_s = ent4.get()
    val5_s = ent5.get()
    val6_s = ent6.get()
    
    if val1_s == "":
        return text_e.set("Wprowadź wagę!")
    if val2_s == "":
        return text_e.set("Wprowadź szerokość opony!")
    if val3_s == "":
        return text_e.set("Wprowadź profil opony!")
    if val5_s == "":
        return text_e.set("Wprowadź początkową wartość RPM!")
    if val6_s == "":
        return text_e.set("Wprowadź końcową wartość RPM!")

    if val4_s == "":
        return text_e.set("Wprowadź szerokość opony!")
    if not is_number(val1_s):
        return text_e.set("Podana masa ma zły format!")
    if not is_number(val2_s):
        return text_e.set("Podana szerokość opony ma zły format!")
    if not is_number(val3_s):
        return text_e.set("Podany profil opony ma zły format!")
    if not is_number(val4_s):
        return text_e.set("Podana średnica felgi ma zły format!")
    if not is_number(val5_s):
        return text_e.set("Podana początkowa wartość RPM ma zły format!")
    if not is_number(val6_s):
        return text_e.set("Podana końcowa wartość RPM ma zły format!")
    text_e.set("")

    val1 = float(val1_s)
    val2 = float(val2_s)
    val3 = float(val3_s)
    val4 = float(val4_s)
    val5 = float(val5_s)
    val6 = float(val6_s)

    if val3 < 0 or val3 > 100:
        return text_e.set("Profil opony powinien być wartością od 0 do 100!")

    global hopony, dkola, obkola, masa, pocz_rpm, kon_rpm
    masa = val1
    pocz_rpm = val5
    kon_rpm = val6

    hopony = val2 * val3 / 100
    dkola = round(val4 * 2.54 * 10 + 2 * hopony, 2)
    obkola = round(2 * math.pi * (dkola/2), 2)

    text2.set(hopony)
    text3.set(dkola)
    text4.set(obkola)

    btn0["state"] = "normal"
    btn1["state"] = "normal"
    lbl_a["text"] = "Proszę wybrać plik z danymi"
    zatwierdzono = True


btn2 = Button(fr1_a, text="Zatwierdź dane", command=buttonClick)
btn2.grid(column=0, row=5)

fr1_a2_b2 = Frame(fr1_a2)
fr1_a2_b2.grid(column=1, row=0, padx=15)

res1_t = Label(fr1_a2_b2, text="Wysokość opony [mm]", font="Verdana 8 bold")
res1_t.pack(pady=5)

text2 = StringVar()
text2.set("0")
res1 = Label(fr1_a2_b2, textvariable=text2)
res1.pack(expand=True)

res2_t = Label(fr1_a2_b2, text="Średnica koła [mm]", font="Verdana 8 bold")
res2_t.pack(pady=5)

text3 = StringVar()
text3.set("0")
res2 = Label(fr1_a2_b2, textvariable=text3)
res2.pack(expand=True)

res1_t = Label(fr1_a2_b2, text="Obwód koła [mm]", font="Verdana 8 bold")
res1_t.pack(pady=5)

text4 = StringVar()
text4.set("0")
res3 = Label(fr1_a2_b2, textvariable=text4)
res3.pack(expand=True)

fr1_a2_b3 = Frame(fr1_a2)
fr1_a2_b3.grid(column=2, row=0, padx=15)

lb5 = Label(fr1_a2_b3, text="Początkowa wartość RPM")
lb5.pack(expand=True)
ent5 = Entry(fr1_a2_b3, width=width)
ent5.pack()

lb6 = Label(fr1_a2_b3, text="Końcowa wartość RPM")
lb6.pack(expand=True)
ent6 = Entry(fr1_a2_b3, width=width)
ent6.pack()

fr1_a2_b4 = Frame(fr1_a2)
fr1_a2_b4.grid(column=3, row=0, padx=15)

res1_t = Label(fr1_a2_b4, text="Konie mechaniczne", font="Verdana 8 bold")
res1_t.pack(pady=5)

text5 = StringVar()
text5.set("0")
res4 = Label(fr1_a2_b4, textvariable=text5)
res4.pack(expand=True)

res1_t = Label(fr1_a2_b4, text="Moment obrotowy ", font="Verdana 8 bold")
res1_t.pack(pady=5)

text6 = StringVar()
text6.set("0")
res5 = Label(fr1_a2_b4, textvariable=text6)
res5.pack(expand=True)

fr1_b = Frame(root)
fr1_b.grid(column=1, row=0)

chart = ChartDrawer([], [], 'black', '', '')
chart.load()

"""
toolbarFrame = Frame(master=root)
toolbarFrame.grid(row=1,column=4)
toolbar = NavigationToolbar2Tk(canvas, toolbarFrame)
"""
root.mainloop()
