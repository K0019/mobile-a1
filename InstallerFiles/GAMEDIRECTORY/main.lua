
Game.LoadTexture("Sprite.png","On");

local go = Game.Create("On")
go:SetPos(300,400);
go:SetColor(1,0,0,1);
--go:SetScale(0.4);
go:SetAngle( math.random(-3.1,3.1) );
go:SetDepth( 1 );

Game.SetTitle("Test Game");
    
--dofile("vector.lua");
Game.SetBackgroundColor( vector(0,0,0,1) );

function MouseDown(x,y)
   
    local go = Game.Create("On")
    go:SetPos(x,y);
    go:SetColor(1,1,1,1);
    go:SetScale(0.4);
    go:SetAngle( -math.pi + math.random() * math.pi * 2 );
    
    
end

function MouseUp()

end

function KeyboardCharacter(character)

	if character == string.byte("f",1) then
		Game.SetSize(1024,768);
	end
	
	if character == string.byte("d",1) then
		Game.SetSize(800,600);
	end
	
end
