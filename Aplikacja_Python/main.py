import math

from matplotlib.backends.backend_tkagg import (FigureCanvasTkAgg, NavigationToolbar2Tk)
from matplotlib.figure import Figure
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

lbl = Label(fr1_a, text="Proszę wybrać plik z danymi", font="Verdana 8 bold")
lbl.grid(column=0, row=0)

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


danex = np.array([])
daney = np.array([])


def load_data():
    if filename == "":
        return False
    global danex
    global daney
    file = open(filename, "r")
    for line in file:
        temp = line.split(";")
        danex = np.append(danex, int(temp[0]))
        daney = np.append(daney, int(temp[1]))
    global canvas, fig, axis1
    axis1.clear()

    axis1.set_ylabel('RPM czujnika', labelpad=15, size=14)
    axis1.set_xlabel('czas [ms]', labelpad=15, size=14)
    axis1.set_title('Wykres hamowania', pad=25, size=20)
    axis1.plot(danex, daney, 'r')

    canvas.draw()


btn0 = Button(fr1_a1, text="Przeglądaj", command=get_file)
btn0.grid(column=0, row=0)

btn0 = Button(fr1_a1, text="Przeładuj", command=load_data)
btn0.grid(column=1, row=0, padx=10)

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
err = Label(fr1_a2_b1, textvariable=text_e)
err.configure(foreground="red")
err.pack(expand=True, pady=2)


def buttonClick():
    val1_s = ent1.get()
    val2_s = ent2.get()
    val3_s = ent3.get()
    val4_s = ent4.get()
    
    if val1_s == "":
        return text_e.set("Wprowadź wagę!")
    if val2_s == "":
        return text_e.set("Wprowadź szerokość opony!")
    if val3_s == "":
        return text_e.set("Wprowadź profil opony!")
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
    text_e.set("")

    val1 = float(val1_s)
    val2 = float(val2_s)
    val3 = float(val3_s)
    val4 = float(val4_s)

    if val3 < 0 or val3 > 100:
        return text_e.set("Profil opony powinien być wartością od 0 do 100!")

    hopony = val2 * val3 / 100
    dkola = round(val4/0.3937 + 2 * hopony, 4)
    obkola = round(2 * math.pi * (dkola/2), 4)

    text2.set(hopony)
    text3.set(dkola)
    text4.set(obkola)


btn1 = Button(fr1_a2_b1, text="Zatwierdź", command=buttonClick)
btn1.pack(pady=10)

fr1_a2_b2 = Frame(fr1_a2)
fr1_a2_b2.grid(column=1, row=0, padx=15)

res1_t = Label(fr1_a2_b2, text="Wysokość opony", font="Verdana 8 bold")
res1_t.pack(pady=5)

text2 = StringVar()
text2.set("0")
res1 = Label(fr1_a2_b2, textvariable=text2)
res1.pack(expand=True)

res2_t = Label(fr1_a2_b2, text="Średnica koła", font="Verdana 8 bold")
res2_t.pack(pady=5)

text3 = StringVar()
text3.set("0")
res2 = Label(fr1_a2_b2, textvariable=text3)
res2.pack(expand=True)

res1_t = Label(fr1_a2_b2, text="Obwód koła", font="Verdana 8 bold")
res1_t.pack(pady=5)

text4 = StringVar()
text4.set("0")
res3 = Label(fr1_a2_b2, textvariable=text4)
res3.pack(expand=True)

fig = Figure(figsize=(8, 8), dpi=60)
axis1 = fig.add_subplot(111)
axis1.set_ylabel('RPM czujnika', labelpad=15, size=14)
axis1.set_xlabel('czas [ms]', labelpad=15, size=14)
axis1.set_title('Wykres hamowania', pad=25, size=20)

#plot, scatter
canvas = FigureCanvasTkAgg(fig, master=root)
canvas.draw()
canvas.get_tk_widget().grid(column=2, row=0, padx=15)

"""
toolbarFrame = Frame(master=root)
toolbarFrame.grid(row=1,column=4)
toolbar = NavigationToolbar2Tk(canvas, toolbarFrame)
"""
root.mainloop()