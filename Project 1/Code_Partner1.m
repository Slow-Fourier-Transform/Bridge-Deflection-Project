clc, clear
% Define elements [node1, node2, L (to be computed), A, E, theta (to be computed)]
nodes = [0 0;5*12 12*sqrt(6^2 -5^2); 10*12 0];
elements = [1 2 0 8.00 1400000 0; % ;
           2 3 0 8.00 1400000 0;
           1 3 0 8.00 1400000 0];
% Initialize global stiffness matrix (6x6)
K_global = zeros(6,6);
% Compute element properties and assemble global stiffness matrix
for i = 1:size(elements,1)
   n1 = elements(i,1);
   n2 = elements(i,2);
   x1 = nodes(n1,1);
   y1 = nodes(n1,2);
   x2 = nodes(n2,1);
   y2 = nodes(n2,2);
  
   % Compute length and angle
   L = hypot(x2-x1, y2-y1);
   theta = atan2(y2-y1, x2-x1);
  
   % Assign length and angle in degrees to the appropriate columns of 'elements'
   elements(i,3) = L;  % Store length L in the 3rd column
   elements(i,6) = rad2deg(theta);  % Store angle theta (in degrees) in the 6th column
  
   % Compute stiffness matrix in local coordinates
   k = (elements(i,4) * elements(i,5)) / L;  % Compute stiffness constant
   c = cos(theta); s = sin(theta);
   T = k * [c^2 c*s -c^2 -c*s;
            c*s s^2 -c*s -s^2;
            -c^2 -c*s c^2 c*s;
            -c*s -s^2 c*s s^2];
  
   % Map local to global stiffness matrix
   dof = [2*n1-1:2*n1, 2*n2-1:2*n2];  % Degrees of freedom
   K_global(dof, dof) = K_global(dof, dof) + T;
end
% Apply boundary conditions and solve for displacements
disp('Global stiffness matrix:');
disp(K_global);
% Apply boundary conditions (Assuming you know which dofs are fixed, for example: nodes 1, 2, and 3 are fixed)
% K_global([1,3,4], :) = [];
% K_global(:, [1,3,4]) = [];
K_global(1,:) = [], K_global(:,1) = []
K_global(1,:) = [], K_global(:,1) = []
K_global(4,:) = [], K_global(:,4) = []
% Solve for displacements (this is a simplified assumption on how the force vector is applied)
Displacements = pinv(K_global) * [0; -1000; 0]  % Example of force vector
%disp('Nodal displacements:');
%disp(Displacements);



