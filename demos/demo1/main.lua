-- Hello Demo
-- Copyright (c) 2021 Dan Wilcox

----- data -----

local lang = {
	{name = "noise",   locale = "xx"},
	{name = "chinese", locale = "zh-cn"},
	{name = "english", locale = "en"},
	{name = "french",  locale = "fr"},
	{name = "german",  locale = "de"},
	{name = "italian",  locale = "it"},
	{name = "russian",  locale = "ru"},
	{name = "spanish", locale = "es"}
}

local greeting = {
	xx = "...",
	zh-cn = "你好", -- Ni Hao
	en = "Hello",
	fr = "Bonjour",
	de = "Guten Tag",
	it = "Ciao",
	ru = "Привет", -- Privet
	es = "Hola"
}

----- text -----

local text = {
	text = "",
	animating = false,
	speed = 100,
	char = 0,
	timestamp = 0,
	font = of.TrueTypeFont(),
	setup = function(self)
		self.font:load(of.TTF_MONO, 64)
	end,
	set = function(self, s)
		self.text = s
		self.char = 0
		self.timestamp = of.getElapsedTimeMillis()
		self.animating = true
	end,
	update = function(self)
		if self.animating then
			if of.getElapsedTimeMillis() - self.timestamp > self.speed then
				self.char = self.char + 1
				if self.char >= #self.text then
					self.char = #self.text
					self.animating = false
				else
					self.timestamp = of.getElapsedTimeMillis()
				end
			end
		end
	end,
	draw = function(self, ddebug)
		local spacing = 4
		local charw = self.font:stringWidth("#")
		local wordw = (charw + spacing) * self.char
		local lineh = self.font:getLineHeight()
		local x = 0
		
		of.pushMatrix()
			of.translate((of.getWidth() / 2) - wordw / 2,
				         (of.getHeight() / 2) - lineh)

			if ddebug then
				of.noFill()
				of.setColor(200, 100, 200)
				of.drawRectangle(0, 0, wordw, lineh)
			end

			of.setColor(64)
			for i=1,self.char do
				self.font:drawString(self.text:sub(i, i), x, lineh)
				x = x + charw + spacing
			end
		of.popMatrix()
	end
}


----- thinking -----

local thinking = {
	text = "?",
	animating = false,
	onoff = false,
	speed = 500,
	timestamp = 0,
	font = of.TrueTypeFont(),
	setup = function(self)
		self.font:load(of.TTF_MONO, 48)
	end,
	show = function(self)
		self.timestamp = of.getElapsedTimeMillis()
		self.animating = true
		self.onoff = true
	end,
	hide = function(self)
		self.animating = false
	end,
	update = function(self)
		if self.animating then
			if of.getElapsedTimeMillis() - self.timestamp > self.speed then
				self.onoff = not self.onoff
				self.timestamp = of.getElapsedTimeMillis()
			end
		end
	end,
	draw = function(self)
		if self.onoff then
			local wordw = self.font:stringWidth(self.text)
			local lineh = self.font:getLineHeight()
			of.setColor(100, 200, 100)
			self.font:drawString(self.text, of.getWidth() - (wordw + 4),
				                            of.getHeight() - 4)
		end
	end
}

----- main -----

local plang = nil
local detecting = false
local confidence = 0
local ddebug = false

function setup()
	of.background(255)

	text:setup()
	thinking:setup()

	loaf.setListenPort(9999)
	loaf.startListening()
end

function update()
	text:update()
	thinking:update()
end

function draw()
	text:draw(ddebug)
	thinking:draw()
	if ddebug then
		if plang then
			of.setColor(100)
			of.drawBitmapString(plang.locale.." "..string.format(" %.2f%%", confidence), 6, 12)
		end
		if detecting then
			of.setColor(100)
			of.drawBitmapString("detecting", of.getWidth() - 78, 12)
		end
	end
end

function keyPressed(key)
	if key >= 49 and key <= 53 then
		local l = lang[key - 48]
		if l ~= plang then
			text:set(greeting[l.locale])
		end
		plang = l
	elseif key == string.byte("d") then
		ddebug = not ddebug
	end
end

function oscReceived(message)
	--print(message)
	if message:getAddress() == "/detecting" then
		detecting = message:getArgAsBool(0)
		if detecting then
			thinking:show()
		else
			thinking:hide()
		end
	elseif message:getAddress() == "/lang" then
		local index = message:getArgAsInt(0) + 1
		confidence = message:getArgAsFloat(2)
		local l = lang[index]
		if l then
			if l ~= plang then
				text:set(greeting[l.locale])
			end
			plang = l
		else
			print("unknown lang: "..tostring(message))
		end
	end
end
