x = linspace(0, 2*pi, 50);
y = 0.5*20.46*sin(x) + 0.5*20.46;
y = round(y)
figure
stairs(y)

%y = 0.5*204*sawtooth(x) + 0.5*204;
%y = round(y)
%stairs(y)

%y = 0.5*204*sawtooth(x, 0.5) + 0.5*204;
%y = round(y)
%stairs(y)