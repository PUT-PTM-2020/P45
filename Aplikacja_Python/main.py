import math
from tkinter import *


def is_number(string):
    for letter in string:
        if not letter.isdigit() and not letter == ".":
            return False
    return True


root = Tk()
root.title("Hamownia drogowa")
root.resizable(False, False)
root.iconbitmap("./tire.ico")

width = 20
height = 1

frl_m1 = Frame(root)
frl_m1.grid(column=0, row=0)

lbl = Label(frl_m1, text="Masa (kg)")
lbl.pack(expand=True)
ent1 = Entry(frl_m1, width=width)
ent1.pack(padx=20)

lb2 = Label(frl_m1, text="Szerokość opony (mm)")
lb2.pack(expand=True)
ent2 = Entry(frl_m1, width=width)
ent2.pack()

lb3 = Label(frl_m1, text="Profil opony (%)")
lb3.pack(expand=True)
ent3 = Entry(frl_m1, width=width)
ent3.pack()

lb4 = Label(frl_m1, text="Średnica felgi (cal)")
lb4.pack(expand=True)
ent4 = Entry(frl_m1, width=width)
ent4.pack()

text_e = StringVar()
err = Label(frl_m1, textvariable=text_e)
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
    dkola = val4/0.3937 + 2 * hopony
    obkola = 2 * math.pi * (dkola/2)

    text2.set(hopony)
    text3.set(dkola)
    text4.set(obkola)


btn1 = Button(frl_m1, text="Zatwierdź", command=buttonClick)
btn1.pack(pady=10)

fr1_m2 = Frame(root)
fr1_m2.grid(column=1, row=0, padx=15)

res1_t = Label(fr1_m2, text="Wysokość opony", font="Verdana 8 bold")
res1_t.pack(pady=5)

text2 = StringVar()
text2.set("0")
res1 = Label(fr1_m2, textvariable=text2)
res1.pack(expand=True)

res2_t = Label(fr1_m2, text="Średnica koła", font="Verdana 8 bold")
res2_t.pack(pady=5)

text3 = StringVar()
text3.set("0")
res2 = Label(fr1_m2, textvariable=text3)
res2.pack(expand=True)

res1_t = Label(fr1_m2, text="Obwód koła", font="Verdana 8 bold")
res1_t.pack(pady=5)

text4 = StringVar()
text4.set("0")
res3 = Label(fr1_m2, textvariable=text4)
res3.pack(expand=True)

root.mainloop()

"""
file = open("dane.txt", "r")

i = 0

for line in file:
    numer = line.split(";")
    for j in range(3):
        txt = Text(root, width=8, height=1)
        txt.insert(INSERT, int(numer[j]))
        txt.config(state=DISABLED)
        txt.configure(font=("Arial", 14))
        txt.grid(row=i, column=j)
    i = i + 1
"""