#include "Player.h"
#include "CollisionCategorie.h"

Player::Player(b2World& physic_world, std::string name) : Actor(physic_world), m_cadence(25.0f), m_timer_fire(1 /m_cadence), m_timer_stun(1.0f)
{
	m_moveSpeed = 15;
	m_isShooting = false;
	m_isLoading = false;
	m_isStun = false;
	m_name = name;


	/* --- Physics settings --- */

	bodyDef.type = b2_dynamicBody;
	bodyDef.position.Set(500.0f / 30.0f, 400.0f / 30.0f);

	body = physic_world.CreateBody(&bodyDef);

	dynamicBox.SetAsBox(50.0f / 60.0f, 50.0f / 60.0f);	/* Box sans friction */

	fixtureDef.shape = &dynamicBox;
	fixtureDef.density = 1.0f;
	fixtureDef.friction = 0.0f;
	fixtureDef.filter.categoryBits = ENEMY_PLAYER;
	bodyDef.fixedRotation = true;
	body->CreateFixture(&fixtureDef);
	body->SetUserData(this);

	m_velocity = b2Vec2(0.0f, 0.0f);

	/* ------------------------- */

	m_Texture.loadFromFile("Assets/player1.png");
	m_Texture.setSmooth(true);
	m_Sprite = sf::Sprite(m_Texture, sf::IntRect(0, 0, 50, 50));
	m_size = b2Vec2(50, 50);
	m_Sprite.setOrigin(m_size.x / 2, m_size.y / 2);

	m_TextureAim.loadFromFile("Assets/aiming_trail.png");
	m_TextureAim.setSmooth(true);
	m_SpriteAim = sf::Sprite(m_TextureAim, sf::IntRect(0, 0, 1500, 5));
	m_SpriteAim.setOrigin(0, 2.5);
}

void Player::getMovement(sf::RenderWindow &window)
{
	m_input.clear();
	

	m_mouseDirection.x = sf::Mouse::getPosition(window).x - m_position.x;
	m_mouseDirection.y = sf::Mouse::getPosition(window).y - m_position.y;

	m_mouseDirection.Normalize(); /* On normalise la vecteur direction */

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Z))
	{
		m_velocity.y = -1 * m_moveSpeed;
		m_input.push_back(TOP);
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
	{
		m_velocity.y = 1 * m_moveSpeed;
		m_input.push_back(BOT);
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q))
	{
		m_velocity.x = -1 * m_moveSpeed;
		m_input.push_back(LEFT);
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
	{
		m_velocity.x = 1 * m_moveSpeed;
		m_input.push_back(RIGHT);
	}
	if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && m_timer_fire.isOver())
	{
		m_isShooting = true;
		if (!m_physic_world.IsLocked())
			bullets.push_back(std::make_unique<Arrow>(b2Vec2(m_position.x + 60 * m_mouseDirection.x, m_position.y + 60 * m_mouseDirection.y), m_mouseDirection, m_physic_world, FRIENDLY_ARROW, ENEMY_ARROW | ENEMY_PLAYER, *this));
		m_input.push_back(LEFT_CLICK);
	}

	if (!sf::Mouse::isButtonPressed(sf::Mouse::Left) && m_isShooting)
	{
		m_isShooting = false;
	}

	if (sf::Mouse::isButtonPressed(sf::Mouse::Right))
	{
		m_isLoading = true;
	}
	if (!(sf::Mouse::isButtonPressed(sf::Mouse::Right)) && m_isLoading)
	{
		m_loadingSprite.reset();
		m_isLoading = false;
		if (m_loadingSprite.isComplete())
		{
			if (!m_physic_world.IsLocked())
				letals.push_back(std::make_unique<LetalShoot>(b2Vec2(m_position.x + 60 * m_mouseDirection.x, m_position.y + 60 * m_mouseDirection.y), m_mouseDirection, m_physic_world, FRIENDLY_ARROW, ENEMY_ARROW | ENEMY_PLAYER, *this));
		}
	}
}

std::vector<std::unique_ptr<Arrow>>& Player::getArrowList()
{
	return bullets;
}

std::vector<std::unique_ptr<LetalShoot>>& Player::getLetalList()
{
	return letals;
}

void Player::update()
{
	body->SetTransform(m_position_server, body->GetAngle());
	m_position.x = m_position_server.x * 30;
	m_position.y = m_position_server.y * 30;
	m_Sprite.setPosition(m_position.x, m_position.y);

	float clamping = 0.75;
	m_moveSpeed = 15;
	b2Vec2 new_velocity = body->GetLinearVelocity(); /* Variable utilis�e seulement si le joueur est stun */ 

	m_Sprite.setRotation(90 - atan2(m_mouseDirection.x, m_mouseDirection.y) * 180 / b2_pi);

	if (m_isShooting)
	{
		m_moveSpeed = 0.35f * m_moveSpeed;
	}

	if (m_isLoading)
	{
		m_loadingSprite.update(sf::Vector2f(m_position.x, m_position.y));
	}
		
	if (m_timer_stun.isOver())
	{
		m_isStun = false;
	}

	/* Update des projectiles classiques */
	for (int i = 0; i < bullets.size(); i++)
	{
		bullets[i]->update();
	}

	/* Update des letals shoots */
	for (int i = 0; i < letals.size(); i++)
	{
		letals[i]->update();
	}


	if (m_isStun) // On laisse un peu plus d'inertie au player si il est stun (pour le bump)
	{
		clamping = 0.85;
	}

	/* Alternative de clamping au lieu de la friction */
	m_velocity.x >= -0.01 && m_velocity.x <= 0.01 ? m_velocity.x = 0 : m_velocity.x = clamping * m_velocity.x; /* Si jamais la velocity est petite */
	m_velocity.y >= -0.01 && m_velocity.y <= 0.01 ? m_velocity.y = 0 : m_velocity.y = clamping * m_velocity.y; /* on la remet � 0				   */
	body->SetLinearVelocity(m_velocity);

	for (int i = 0; i < bulletsToCreate.size(); i+=2)
	{
		bullets.push_back(std::make_unique<Arrow>(b2Vec2(bulletsToCreate[i].x, bulletsToCreate[i].y), bulletsToCreate[i + 1], m_physic_world, FRIENDLY_ARROW, ENEMY_ARROW | ENEMY_PLAYER, *this));
	}
	bulletsToCreate.clear();
}

void Player::draw(sf::RenderTarget &window)
{
	Actor::draw(window);
	
	for (int i = 0; i < bullets.size(); i++)
	{
		bullets[i]->draw(window);
	}

	for (int i = 0; i < letals.size(); i++)
	{
		letals[i]->draw(window);
	}

	if (m_isLoading)
	{
		m_loadingSprite.draw(window);
		m_SpriteAim.setPosition(m_position.x, m_position.y);
		m_SpriteAim.setRotation(90 - atan2(m_mouseDirection.x, m_mouseDirection.y) * 180 / b2_pi);
		window.draw(m_SpriteAim);
	}
}

void Player::bump(b2Vec2 velocity, float force)
{
	m_isStun = true;
	m_timer_stun.refresh();
	m_velocity = b2Vec2(velocity.x * force, velocity.y * force);
}

bool Player::isAlive()
{
	return m_isAlive;
}

std::vector<Input> Player::getInputsList()
{
	return m_input;
}

bool Player::hasInputs()
{
	return m_input.size() > 0;

}

sf::Packet& operator <<(sf::Packet& packet, Player& character)
{
	packet << character.m_mouseDirection.x << character.m_mouseDirection.y << character.m_isShooting << character.m_isLoading;

	for (int i = 0; i < character.m_input.size(); i++)
	{
		packet << character.m_input[i];
	}
	return packet;
}

void Player::extractDataPlayer(sf::Packet packet)
{
	b2Vec2 position;
	packet >> position.x >> position.y;
	m_position_server = position;
}

void Player::extractDataEnnemyPlayer(sf::Packet packet)
{
	b2Vec2 position;
	packet >> position.x >> position.y;
	m_position_server = position;
	b2Vec2 mouseDirection;
	packet >> mouseDirection.x >> mouseDirection.y;
	m_mouseDirection = mouseDirection;
	
	while (!packet.endOfPacket())
	{
		b2Vec2 position_bullet;
		b2Vec2 velocity;

		packet >> position_bullet.x >> position_bullet.y;
		position_bullet.x *= 30.0f;
		position_bullet.y *= 30.0f;
		packet >> velocity.x >> velocity.y;
		
		bulletsToCreate.push_back(position_bullet);
		bulletsToCreate.push_back(m_mouseDirection);
	}
}